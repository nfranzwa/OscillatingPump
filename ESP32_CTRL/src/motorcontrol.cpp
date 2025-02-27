#include <MightyZap.h>
#include <Arduino.h>
#include <sharedData.h>
#include <motorcontrol.h>

// ESP32 has multiple hardware serial ports (UART)
// We'll use Serial2 for MightyZap communication
int maxPosition = 2000;           // Default max position
float frequency = 0.75;           // Default frequency in Hz
unsigned long lastSensorRead = 0; // For timing sensor readings
unsigned long cycleStartTime = 0; // For tracking cycle timing
const float DUTY_CYCLE = 0.4;     // 40% duty cycle

// Calculate speed needed for symmetric movement within duty cycle
int MotorControl::calculateSpeed(int distance, float freq)
{
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

void MotorControl::updateParameters()
{
  int distance = maxPosition - MIN_POS;
  int speed = calculateSpeed(distance, frequency);
  m_zap->GoalSpeed(ID_NUM, speed);
  Serial.printf("Updated speed to: %d\n", speed);
}

void MotorControl::begin()
{
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

// as long as the target pressure is within the calibration range, run a binary search
// WARNING: binary search assumes the values sorted (aka monotonic)
int mapPos(float P_target) {
  if(P_target==sharedData.P_max) return sharedData.PWM_c_max;
  if(P_target==sharedData.P_min) return sharedData.PWM_c_min;
  if(P_target<sharedData.P_max || P_target>sharedData.P_min){
    int left = 0, right = 4094;  // Search space
    int i=0; // for debugging
    int closestPos = 0;
    float minDiff = fabs(sharedData.pmap[0] - P_target);
    while (left <=right) {
      int mid = left + (right - left) / 2;
      float midValue = sharedData.pmap[mid];
      // Update closest position if current mid is closer
      float diff = fabs(midValue - P_target);
      if (diff < minDiff) {
        minDiff = diff;
        closestPos = mid;
      }

      // Move search window
      if (midValue < P_target) {
        left = mid + 1;
      } else {
        right = mid - 1;
      }
      i++; //this is for debugging
      if(i>12) {
        Serial.println("Binary search error, too many loops. Returning current guess\n");
        return closestPos;
      }
    }
    return closestPos;
  }
  else{
    Serial.println("Target pressure out of range, recalibrate\n");
  }
}
//TODO ? maybe not necessary
void resetMap(){
  // memcpy(sharedData.pmap, something to change it to)
}

/*
calibrate pressure mappings and motor
this means remembering
- starting PWM position for 1 atm
- positions for up to max pressure
*/
void TF_calibrate(void* pvParams){
  MotorControl* motor = (MotorControl*) pvParams;
  //set move speed to be very very slow
  for(;;){
    if(sharedData.calibration_state==1){
      Serial.println("Starting calibration, Detach Pump\n");
      vTaskDelay(pdMS_TO_TICKS(800));
      motor->m_zap->GoalPosition(3000);
      motor->m_zap->GoalSpeed(ID_NUM,10);
      Serial.println("Reattach Pump\n");
      vTaskDelay(pdMS_TO_TICKS(1500));
      //first move down until slightly below 0.1 KPa
      /*
      WARNING: Know that increasing PWM values corresponds with minimizing pressure.
      For the sake of convention + retained understanding, we do the flip when
      - sending information to the motor.
      - updating the values of targets
      */
      while(sharedData.P_current>sharedData.P_min){
        motor->m_zap->GoalPosition(ID_NUM,4095-sharedData.PWM_min);
      }
      sharedData.PWM_c_min=4095-motor->m_zap->presentPosition(ID_NUM);
      //start mapping, and then move until it hits max desired pressure
      while(sharedData.P_current<sharedData.P_max){
        motor->m_zap->GoalPosition(ID_NUM,sharedData.PWM_max);
        sharedData.pmap[motor->m_zap->presentPosition(ID_NUM)] = sharedData.P_current;
      }
      sharedData.PWM_c_max=4095-motor->m_zap->presentPosition(ID_NUM);
      
      //reset move speed to default? max is 1023
      motor->m_zap->GoalSpeed(ID_NUM,500);
      motor->m_zap->GoalPosition(ID_NUM,4095-sharedData.PWM_c_min);
      sharedData.calibration_state=2;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void TF_motor(void* pvParams){
  MotorControl* motor = (MotorControl*) pvParams;
  const int moveSpeed = 50;
  const int stopSpeed = 0;
  for (;;)
  {
    // while motor is calibrated, constantly be moving motor
    if(sharedData.calibration_state==2){
      motor->m_zap->GoalPosition(ID_NUM, 4095 - sharedData.PWM_value);
    }
    vTaskDelay(pdMS_TO_TICKS(3));
  }
}

void mapPressure(void* pvParams)
{
  MotorControl* motor = (MotorControl*) pvParams;
  for (;;)
  {
    sharedData.pmap[4095-motor->m_zap->presentPosition(ID_NUM)] = sharedData.P_current;
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}