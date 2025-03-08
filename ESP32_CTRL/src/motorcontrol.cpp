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
    if(sharedData.calibration_state==3){
      motor->m_zap->GoalPosition(ID_NUM,4095-sharedData.PWM_manual);
    }
    sharedData.PWM_current=motor->m_zap->presentPosition(ID_NUM);
    vTaskDelay(pdMS_TO_TICKS(4));
  }
}

/*
calibrate pressure mappings and motor
this means remembering
- starting PWM position for 1 atm
- positions for up to max pressure
*/
void TF_calibrate(void* pvParams){
  MotorControl* motor = (MotorControl*) pvParams;
  Serial.println("Cal Task");
  //set move speed to be very very slow
  //0: hasn't started
  //1: retract to min sensor reading
  //2: finding P_min
  //3: calibrating pmap until reaching P_max
  //4: reached P_max
  int stage=0;
  //first move down until slightly below 0.1 KPa
  /*
  WARNING: Know that increasing PWM values corresponds with minimizing pressure.
  For the sake of convention + retained understanding, we do the flip when
  - sending information to the motor.
  - updating the values of targets
  */
  //start mapping, and then move until it hits max desired pressure
  for(;;){

    //Serial.printf("Calibration state:%d\t",sharedData.calibration_state);
    if(sharedData.calibration_state==0) stage=0;
    if(sharedData.calibration_state==1){
      switch (stage){
        case 0: //hasn't started yet
          Serial.println("Starting calibration, Detach Pump\n");
          vTaskDelay(pdMS_TO_TICKS(800));
          motor->m_zap->GoalPosition(3000);
          motor->m_zap->GoalSpeed(ID_NUM,10);
          Serial.println("Reattach Pump\n");
          vTaskDelay(pdMS_TO_TICKS(1500));
          stage++;
          break;
        case 1: //move back until minimum sensor reading
          //make sure that P_current (output of sensor) can reach below P_min
          Serial.println("Retracting until hitting minmium sensor pressure\n");
          if(sharedData.P_current>=sharedData.P_min){
            motor->m_zap->GoalPosition(ID_NUM,4095-sharedData.PWM_min);
          }
          else{
            Serial.printf("Retracted to %d\n",motor->m_zap->presentPosition(ID_NUM));
            motor->m_zap->GoalPosition(ID_NUM,motor->m_zap->presentPosition(ID_NUM));
            stage++;
          }
          break;
        case 2: //find P_min
          if(sharedData.P_current<sharedData.P_min){
            Serial.printf("Finding P_min\n");
            motor->m_zap->GoalPosition(ID_NUM,4095-sharedData.PWM_min);
          }
          else{
            sharedData.PWM_c_min=4095-motor->m_zap->presentPosition(ID_NUM);
            motor->m_zap->GoalPosition(ID_NUM,motor->m_zap->presentPosition(ID_NUM));
            sharedData.PWM_last_min=sharedData.PWM_c_min;
            stage++;
            Serial.printf("Found min @%d\n",sharedData.PWM_c_min);
          }
          break;
        case 3: //sweep to P_max
          if(sharedData.P_current<sharedData.P_max){
            Serial.printf("Sweeping to P_max\n");
            motor->m_zap->GoalPosition(ID_NUM,sharedData.PWM_max);
            sharedData.pmap[motor->m_zap->presentPosition(ID_NUM)] = sharedData.P_current;
          }
          else{
            sharedData.PWM_c_max=4095-motor->m_zap->presentPosition(ID_NUM);
            stage++;
            Serial.printf("P_max found @ %d\n",sharedData.PWM_c_max);
          }
          break;
        default:
          Serial.printf("WARNING: Default case, stage%d\n",stage);
          Serial.println("Resetting Speed and returning to P_min\n");
          //reset move speed to default? max is 1023
          motor->m_zap->GoalSpeed(ID_NUM,500);
          motor->m_zap->GoalPosition(ID_NUM,4095-sharedData.PWM_c_min);
          sharedData.calibration_state=2;
          break;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}