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

void MotorControl::readAndPrintSensor() {
  int analogValue = analogRead(ANALOG_PIN);
  float voltage = (analogValue * 3.3) / 4095.0;  // Convert to voltage
  
  // Map analog value to pressure (mbar)
  float pressure_mbar = map(analogValue, 0, 4095, 0, 60);
  
  // Convert mbar to PSI (1 mbar = 0.0145038 PSI)
  float pressure_psi = pressure_mbar * 0.0145038;
  
  Serial.printf("Raw: %d, Voltage: %.2fV, Pressure: %.2f mbar (%.3f PSI)\n", 
                analogValue, voltage, pressure_mbar, pressure_psi);
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

void TF_motor(void* pvParams) {
  MotorControl* motor= (MotorControl*) pvParams;
  const int moveSpeed = 50;
  const int stopSpeed = 0;
  
  for(;;) {
    // Set goal to max position
    motor->m_zap->GoalPosition(ID_NUM, sharedData.PWM_value);
    vTaskDelay(pdMS_TO_TICKS(3));
  }
}