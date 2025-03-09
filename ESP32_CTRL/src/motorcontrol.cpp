#include <MightyZap.h>
#include <Arduino.h>
#include <sharedData.h>
#include <motorcontrol.h>
#define ID_NUM 0
#define MIN_POS 100

// ESP32 has multiple hardware serial ports (UART)
// We'll use Serial2 for MightyZap communication
int maxPosition = 2000;  // Default max position
float frequency = 0.75;   // Default frequency in Hz
unsigned long lastSensorRead = 0;  // For timing sensor readings
unsigned long cycleStartTime = 0;  // For tracking cycle timing
const float DUTY_CYCLE = 0.4;  // 40% duty cycle

// Calculate speed needed for symmetric movement within duty cycle
int MotorControl::calculateSpeed(int distance, float freq) {
  // Total period = 1/frequency
  float totalPeriod = 1.0 / freq;
  
  // Active time is 40% of the period, split equally between forward and back
  float activeTime = totalPeriod * DUTY_CYCLE;
  // Each movement (forward or back) gets half of the active time
  float moveTime = activeTime / 2.0;
  
  // Calculate required speed in MightyZap units (mm/sec * 10)
  float requiredSpeed = (float)distance / moveTime / 10.0;
  
  // Constrain to valid speed range
  return (int)constrain(requiredSpeed, 1, 1023);
}

void MotorControl::updateParameters() {
  int distance = maxPosition - MIN_POS;
  int speed = calculateSpeed(distance, frequency);
  m_zap->GoalSpeed(ID_NUM, speed);
  Serial.printf("Updated speed to: %d\n", speed);
}


void MotorControl::begin() {
  // Initialize Serial2 for MightyZap communication
  // Serial2.begin(32, SERIAL_8N1, MIGHTY_ZAP_RX, MIGHTY_ZAP_TX);
  
  // Initialize MightyZap
  m_zap->begin(32);
  
  // Set ADC resolution to 12 bits
  // analogSetWidth(12);
  
  updateParameters();
  cycleStartTime = millis();
  int currentPos = m_zap->presentPosition(ID_NUM);
  Serial.printf("Initial position: %d\n", currentPos);
}

/*
only moves motor to target position under specific calibration state
*/
void TF_motor(void* pvParams) {
  MotorControl* motor= (MotorControl*) pvParams;
  Serial.println("Motor Task");
  for(;;) {
    if(sharedData.calibration_state==2){
      motor->m_zap->GoalPosition(ID_NUM, 4095 - sharedData.PWM_value);
    }
    else if(sharedData.calibration_state==3){
      motor->m_zap->GoalPosition(ID_NUM,4095-sharedData.PWM_manual);
    }
    // sharedData.PWM_current=motor->m_zap->presentPosition(ID_NUM);
    vTaskDelay(pdMS_TO_TICKS(4));
  }
}

/*
calibrate pressure mappings and motor
this means remembering
- starting PWM position for 1 atm
- positions for up to max pressure
*/
/*
 * Calibrate pressure mappings and motor
 * This means remembering:
 * - starting PWM position for 1 atm
 * - positions for up to max pressure
 */
void TF_calibrate(void* pvParams) {
  bool debug=true;
  MotorControl* motor = (MotorControl*) pvParams;
  int stage = 0;
  unsigned long lastCommandTime = 0;
  bool commandSent = false;
  int currentPosition = 0;
  int targetPosition = 0;
  
  // Register with watchdog
  #ifdef CONFIG_ESP_TASK_WDT_ENABLED
      esp_task_wdt_add(NULL);
  #endif
  
  Serial.println("Calibration Task Started");
  
  for(;;) {
      if(debug) {
          Serial.printf("Calibration state: %d, Stage: %d\n", 
              sharedData.calibration_state, stage);
      }
      
      // Reset stage if calibration state is reset
      if(sharedData.calibration_state == 0) {
          stage = 0;
          commandSent = false;
      }
      
      if(sharedData.calibration_state == 1) {
          switch(stage) {
            case 0: // Initialization
              // Only print message once
              if(!commandSent) {
                  Serial.println("Starting calibration. Please detach pump.");
                  commandSent = true;
                  lastCommandTime = millis();
              }
              
              // Wait for user to detach pump
              if(millis() - lastCommandTime > 3000) {
                  // Send the position command only once
                  if(commandSent) {
                      targetPosition = 3000;
                      // Use direct command to verify it's sent properly
                      motor->m_zap->GoalPosition(ID_NUM, targetPosition);
                      int startrange=30;
                      if(sharedData.PWM_value<=targetPosition+startrange
                      || sharedData.PWM_value>=targetPosition-startrange) {
                          Serial.printf("Command sent to move to position %d\n", targetPosition);
                          lastCommandTime = millis();
                          commandSent = false; // Reset for next command
                      } else {
                          Serial.println("ERROR: Failed to send motor command!");
                      }
                  }
                  
                  // Check if motor has reached target or timeout
                  currentPosition = motor->m_zap->presentPosition(ID_NUM);
                  if(abs(currentPosition - targetPosition) < 10 || millis() - lastCommandTime > 5000) {
                      Serial.println("Motor reached position or timeout. Please reattach pump.");
                      motor->m_zap->GoalSpeed(ID_NUM, 10); // Set to slow speed
                      lastCommandTime = millis();
                      stage++;
                      commandSent = false;
                  }
              }
              break;
            case 1: // Move back until minimum sensor reading
                if(millis() - lastCommandTime > 4000) { // Ensure pump has been attached
                    if(!commandSent) {
                        Serial.println("Retracting until hitting minimum sensor pressure");
                        commandSent = true;
                    }
                    
                    // Check sensor reading
                    if(sharedData.P_current >= sharedData.P_min) {
                        // Still above min pressure, continue moving
                        targetPosition = 4095 - sharedData.PWM_min;
                        motor->m_zap->GoalPosition(ID_NUM, targetPosition);
                    } else {
                        // Reached target pressure
                        currentPosition = motor->m_zap->presentPosition(ID_NUM);
                        Serial.printf("Retracted to position %d\n", currentPosition);
                        // Hold current position
                        motor->m_zap->GoalPosition(ID_NUM, currentPosition);
                        stage++;
                        commandSent = false;
                    }
                }
                break;
                
            case 2: // Find P_min position precisely
                if(!commandSent) {
                    Serial.printf("Finding precise P_min (%2.2f) position, current pressure: %.2f\n", 
                        sharedData.P_min, sharedData.P_current);
                    commandSent = true;
                }
                
                if(sharedData.P_current > sharedData.P_min) {
                    // Fine adjustment to find exact pressure
                    targetPosition = 4095 - sharedData.PWM_min;
                    motor->m_zap->GoalPosition(ID_NUM, targetPosition);
                } else {
                    // Save the position where P_min is achieved
                    currentPosition = motor->m_zap->presentPosition(ID_NUM);
                    sharedData.PWM_c_min = 4095 - currentPosition;
                    sharedData.PWM_last_min = sharedData.PWM_c_min;
                    
                    // Hold current position
                    motor->m_zap->GoalPosition(ID_NUM, currentPosition);
                    
                    Serial.printf("Found P_min at PWM value %d\n", sharedData.PWM_c_min);
                    stage++;
                    commandSent = false;
                }
                break;
                
            case 3: // Sweep to P_max while recording pressure map
                if(!commandSent) {
                    Serial.println("Sweeping to P_max and building pressure map");
                    commandSent = true;
                }
                
                if(sharedData.P_current < sharedData.P_max) {
                    // Move gradually toward PWM_max
                    currentPosition = motor->m_zap->presentPosition(ID_NUM);
                    
                    // Record current pressure value in the mapping
                    int pwm_value = 4095 - currentPosition;
                    if(pwm_value >= sharedData.PWM_min && pwm_value <= sharedData.PWM_max) {
                        sharedData.pmap[pwm_value] = sharedData.P_current;
                    }
                    
                    // Move slowly to build a detailed mapping
                    targetPosition = currentPosition - 1; // Small incremental movement
                    if(targetPosition < sharedData.PWM_max) {
                        targetPosition = sharedData.PWM_max;
                    }
                    motor->m_zap->GoalPosition(ID_NUM, targetPosition);
                } else {
                    // Reached or exceeded P_max
                    currentPosition = motor->m_zap->presentPosition(ID_NUM);
                    sharedData.PWM_c_max = 4095 - currentPosition;
                    
                    Serial.printf("Found P_max at PWM value %d\n", sharedData.PWM_c_max);
                    stage++;
                    commandSent = false;
                }
                break;
                
            default:
                if(!commandSent) {
                    Serial.println("Calibration complete. Resetting speed and returning to P_min position");
                    // Reset move speed to default (500)
                    motor->m_zap->GoalSpeed(ID_NUM, 500);
                    // Return to P_min position
                    targetPosition = 4095 - sharedData.PWM_c_min;
                    motor->m_zap->GoalPosition(ID_NUM, targetPosition);
                    
                    commandSent = true;
                }
                
                // Wait for motor to reach position
                currentPosition = motor->m_zap->presentPosition(ID_NUM);
                if(abs(currentPosition - targetPosition) < 10 || millis() - lastCommandTime > 5000) {
                    // Mark calibration as complete
                    sharedData.calibration_state = 2;
                    stage = 0;
                    commandSent = false;
                }
                break;
          }
      }
      
      // Yield to other tasks
      vTaskDelay(pdMS_TO_TICKS(20));
  }
}
