#include <MightyZap.h>
#define ID_NUM 0
#define MIN_POS 100

// ESP32 has multiple hardware serial ports (UART)
// We'll use Serial2 for MightyZap communication
#define MIGHTY_ZAP_RX 16  // Define RX pin for Serial2
#define MIGHTY_ZAP_TX 17  // Define TX pin for Serial2
#define MIGHTY_ZAP_EN 2   // Enable pin
#define ANALOG_PIN 34     // Analog input pin

// Initialize MightyZap with Serial2 and timeout of 1
MightyZap m_zap(&Serial2, MIGHTY_ZAP_EN, 1);

int maxPosition = 2000;  // Default max position
float frequency = 0.75;   // Default frequency in Hz
unsigned long lastSensorRead = 0;  // For timing sensor readings
unsigned long cycleStartTime = 0;  // For tracking cycle timing
const float DUTY_CYCLE = 0.4;  // 40% duty cycle

// Calculate speed needed for symmetric movement within duty cycle
int calculateSpeed(int distance, float freq) {
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

void updateParameters() {
  int distance = maxPosition - MIN_POS;
  int speed = calculateSpeed(distance, frequency);
  m_zap.GoalSpeed(ID_NUM, speed);
  Serial.printf("Updated speed to: %d\n", speed);
}

void readAndPrintSensor() {
  int analogValue = analogRead(ANALOG_PIN);
  float voltage = (analogValue * 3.3) / 4095.0;  // Convert to voltage
  
  // Map analog value to pressure (mbar)
  float pressure_mbar = map(analogValue, 0, 4095, 0, 60);
  
  // Convert mbar to PSI (1 mbar = 0.0145038 PSI)
  float pressure_psi = pressure_mbar * 0.0145038;
  
  Serial.printf("Raw: %d, Voltage: %.2fV, Pressure: %.2f mbar (%.3f PSI)\n", 
                analogValue, voltage, pressure_mbar, pressure_psi);
}

void setup() {
  // Initialize USB Serial for debugging and commands
  Serial.begin(115200);  // Changed to 115200 for faster data transfer
  
  // Initialize Serial2 for MightyZap communication
  Serial2.begin(32, SERIAL_8N1, MIGHTY_ZAP_RX, MIGHTY_ZAP_TX);
  
  // Initialize MightyZap
  m_zap.begin(32);
  
  // Set ADC resolution to 12 bits
  analogSetWidth(12);
  
  updateParameters();
  cycleStartTime = millis();
}

void loop() {
  // Read sensor regularly
  if (millis() - lastSensorRead >= 25) {
    readAndPrintSensor();
    lastSensorRead = millis();
  }

  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    
    if (command.startsWith("P:")) {
      maxPosition = command.substring(2).toInt();
      updateParameters();
      Serial.printf("Max position set to: %d\n", maxPosition);
    }
    else if (command.startsWith("F:")) {
      frequency = command.substring(2).toFloat();
      updateParameters();
      Serial.printf("Frequency set to: %.2f Hz\n", frequency);
    }
  }
  
  // Calculate cycle timing
  float totalPeriod = 1000.0 / frequency;  // Period in milliseconds
  float activeTime = totalPeriod * DUTY_CYCLE;  // Active time in milliseconds
  float restTime = totalPeriod * (1.0 - DUTY_CYCLE);  // Rest time in milliseconds
  unsigned long currentTime = millis() - cycleStartTime;
  
  // Reset cycle if needed
  if (currentTime >= totalPeriod) {
    cycleStartTime = millis();
    currentTime = 0;
  }
  
  // Active period (40% of cycle)
  if (currentTime < activeTime) {
    // First half of active period: forward movement
    if (currentTime < activeTime/2) {
      m_zap.GoalPosition(ID_NUM, MIN_POS);
    } 
    // Second half of active period: backward movement
    else {
      m_zap.GoalPosition(ID_NUM, maxPosition);
    }
  }
  // Rest period (60% of cycle)
  else {
    // Hold current position during rest period
    if (!m_zap.Moving(ID_NUM)) {
      delay(10);  // Small delay to prevent CPU hogging
    }
  }
  
  // Continue sensor readings during movement
  while(m_zap.Moving(ID_NUM)) {
    if (millis() - lastSensorRead >= 25) {
      readAndPrintSensor();
      lastSensorRead = millis();
    }
    delay(10);
  }
}