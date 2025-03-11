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

/**
 * @brief Calibrate initial motor position and pressure estimates
 * Stage 0: Initial message (detach pump)
 * Stage 1: Moving to initial position
 * Stage 2: Waiting for pump reattachment
 * Stage 3: Retracting to minimum pressure
 * Stage 4: Finding precise P_min position
 * Stage 5: Sweeping to P_max
 * Stage 6: Finalizing calibration
 * 
 * @param pvParams The MightyZap motor address
 */
void TF_calibrate(void* pvParams) {
  bool debug = false;
  MotorControl* motor = (MotorControl*) pvParams;
  int stage = 0;
  unsigned long lastCommandTime = 0;
  bool commandSent = false;
  int currentPosition = 0;
  int targetPosition = 0;
  int startrange=20;
  // Register with watchdog
  #ifdef CONFIG_ESP_TASK_WDT_ENABLED
      esp_task_wdt_add(NULL);
  #endif
  
  Serial.println("Calibration Task Started");
  for(;;) {
    debug = sharedData.cal_debug;
    if(debug) {
      Serial.printf("state: %d, Stage: %d, CMD time: %5f\n", 
        sharedData.calibration_state, stage, 
        sharedData.calibration_state==2 ? 0: (float) (millis()-lastCommandTime));
    }
    
    // Reset stage if calibration state is reset
    if(sharedData.calibration_state == 0) {
      stage = 0;
      commandSent = false;
      int idle_PWM=2000;
      sharedData.PWM_value=motor->m_zap->presentPosition(ID_NUM);
      if(sharedData.PWM_value>=idle_PWM+10 || sharedData.PWM_value<=idle_PWM-10) motor->m_zap->GoalPosition(ID_NUM,idle_PWM);
    }
    // running calibration
    if(sharedData.calibration_state == 1) {
      switch(stage) {
        case 0: // Initialization
          // Only print message once
          if(!commandSent) {
            Serial.println("Starting calibration. Please detach pump.");
            commandSent = true;
            lastCommandTime = millis();
          }
          
          // Wait 3 seconds for user to detach pump
          if(millis() - lastCommandTime > 3000) {
            // Move to next step
            stage = 1;
            commandSent = false;
            if(debug) Serial.printf("End of stage 0, moving to stage 1\n");
          }
          break;
          
        case 1: // Move to initial position
          if(!commandSent) {
            Serial.println("Moving to initial position");
            targetPosition = 2500;
            motor->m_zap->GoalPosition(ID_NUM, targetPosition);
            commandSent = true;
            lastCommandTime = millis();
          }
          
          // Check if reached position or timeout
          sharedData.PWM_value = 4095 - motor->m_zap->presentPosition(ID_NUM);
          if(abs(sharedData.PWM_value - targetPosition) <= startrange || millis() - lastCommandTime > 3000) {
            if(debug) Serial.printf("Retract CMD fulfilled: %d\n", sharedData.PWM_value);
            Serial.println("Motor reached position or timeout. Please reattach pump.");
            lastCommandTime = millis();
            stage = 2;
            commandSent = false;
            if(debug) Serial.printf("End of stage 1, moving to stage 2\n");
          }
          break;
          
        case 2: // Wait for user to reattach pump
          if(millis() - lastCommandTime > 3500) {
            stage = 3;
            commandSent = false;
            if(debug) Serial.printf("End of stage 2, moving to stage 3\n");
          }
          break;
          
        case 3: // Move back until minimum sensor reading
          if(!commandSent) {
            if(debug) Serial.println("Send CMD: retract until Pmin");
            commandSent = true;
          }
          
          // retract if over pressurized
          if(sharedData.P_current >= sharedData.P_min) {
            if(debug) Serial.println("Retracting until hitting minimum sensor pressure");
            // Still above min pressure, continue moving
            targetPosition = 4095;
            motor->m_zap->GoalPosition(ID_NUM, targetPosition);
            
          } else {
            // Reached target pressure
            sharedData.PWM_value = 4095 - motor->m_zap->presentPosition(ID_NUM);
            if(debug) Serial.printf("retract CMD finished: P_min ~ %d\n", sharedData.PWM_value);
            motor->m_zap->GoalPosition(ID_NUM, 4095-sharedData.PWM_value);
            motor->m_zap->GoalSpeed(ID_NUM, 10); // Set to slow speed
            if(debug) Serial.printf("Minimized speed, end of stage 3\n");
            stage = 4;
            commandSent = false;
          }
          break;
          
        case 4: // Find P_min position precisely
          if(!commandSent) {
            if(debug) Serial.printf("Finding precise P_min (%2.2f) position, current pressure: %.2f\n", 
              sharedData.P_min, sharedData.P_current);
            commandSent = true;
            if(debug) Serial.printf("RESETTING PMAP to -1\n");
            sharedData.pmap.fill(-1.0);
          }
          
          // if still over pressure  
          if(sharedData.P_current > sharedData.P_min) {
            // set target to be full extended
            if(debug) Serial.println("Still over P");
            motor->m_zap->GoalPosition(ID_NUM, 4095);
            if(motor->m_zap->presentPosition(ID_NUM)>=4090){
              Serial.println("Unable to retract fully");
              sharedData.calibration_state=4;
              sharedData.error=1;
            }
          } 
          else { // under or equal to Pmin
            // Save the position where P_min is achieved
            sharedData.PWM_value = 4095 - motor->m_zap->presentPosition(ID_NUM);
            sharedData.PWM_c_min = sharedData.PWM_value;
            sharedData.PWM_last_min = sharedData.PWM_c_min;
            // Hold current position
            motor->m_zap->GoalPosition(ID_NUM, 4095-sharedData.PWM_value);
            if(debug) Serial.printf("Found P_min at PWM value %d\n", sharedData.PWM_c_min);
            stage = 5;
            commandSent = false;
          }
          break;
          
        case 5: // Sweep to P_max while recording pressure map
          if(!commandSent) {
            Serial.println("Sweeping to P_max and building pressure map");
            commandSent = true;
          }
          
          if(sharedData.P_current < sharedData.P_max) {
            // Move gradually toward PWM_max
            sharedData.PWM_value = 4095 - motor->m_zap->presentPosition(ID_NUM);
            
            // Record current pressure value in the mapping
            sharedData.pmap[sharedData.PWM_value] = sharedData.P_current;
            if(debug) Serial.printf("Making map: pmap[%4d]=%+2.3f\n",
              sharedData.PWM_value, sharedData.pmap[sharedData.PWM_value]);
            // Move slowly to build a detailed mapping
            motor->m_zap->GoalPosition(ID_NUM, 0);
            if(sharedData.PWM_value>=10){
              Serial.println("Unable to reach P_max");
              sharedData.calibration_state=4;
              sharedData.error=1;
            }
          } else { // Reached or exceeded P_max
            sharedData.PWM_value = 4095 - motor->m_zap->presentPosition(ID_NUM);
            sharedData.PWM_c_max = sharedData.PWM_value;
            if(debug) Serial.printf("Found P_max at PWM value %d\n", sharedData.PWM_c_max);
            stage = 6;
            commandSent = false;
          }
          break;
          
        case 6: // Finalize calibration
          if(!commandSent) {
            if(debug) Serial.println("Ending calibration. Resetting speed and returning to P_min position");
            motor->m_zap->GoalSpeed(ID_NUM, 800);
            // Return to P_min position
            motor->m_zap->GoalPosition(ID_NUM, 4095-sharedData.PWM_c_min);
            commandSent = true;
            lastCommandTime = millis();
          }
          
          currentPosition = 4095 - motor->m_zap->presentPosition(ID_NUM);
          targetPosition = sharedData.PWM_c_min;
          
          if(abs(currentPosition - targetPosition) < 10 || millis() - lastCommandTime > 5000) {
            // Clean the calibration array
            cleanArray(sharedData.pmap, sharedData.PWM_c_min, sharedData.PWM_c_max);
            
            // Mark calibration as complete
            if(debug) Serial.println("CAL STATE=2");
            sharedData.calibration_state = 2;
            stage = 0;
            commandSent = false;
            sharedData.cal_debug=false;
          }
          break;
      }
    }
    // Yield to other tasks
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}