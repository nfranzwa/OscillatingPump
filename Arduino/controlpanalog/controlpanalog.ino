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

int maxPosition = 2500;  // Default max position
float frequency = 1.0;   // Default frequency in Hz
unsigned long lastSensorRead = 0;  // For timing sensor readings

// Calculate required speed for 50% duty cycle
int calculateSpeed(int distance, float freq) {
  // Speed in MightyZap units (0-1023) needed to cover the distance in half the period
  // Period = 1/frequency, half period = 1/(2*frequency)
  // MightyZap speed units are approximately mm/sec * 10
  float timePerCycle = 1.0 / (2.0 * freq);  // Time for one direction (half cycle)
  float requiredSpeed = (float)distance / timePerCycle / 10.0;  // Convert to MightyZap units
  return (int)constrain(requiredSpeed, 1, 1023);
}

void updateParameters() {
  int distance = maxPosition - MIN_POS;
  int speed = calculateSpeed(distance, frequency);
  m_zap.GoalSpeed(ID_NUM, speed);
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
}

void loop() {
  // Read sensor every 100ms
  if (millis() - lastSensorRead >= 100) {
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
  
  // Oscillate between min and max positions
  m_zap.GoalPosition(ID_NUM, MIN_POS);
  while(m_zap.Moving(ID_NUM)) {
    // Continue reading sensor while waiting
    if (millis() - lastSensorRead >= 100) {
      readAndPrintSensor();
      lastSensorRead = millis();
    }
    delay(10);
  }
  
  m_zap.GoalPosition(ID_NUM, maxPosition);
  while(m_zap.Moving(ID_NUM)) {
    // Continue reading sensor while waiting
    if (millis() - lastSensorRead >= 100) {
      readAndPrintSensor();
      lastSensorRead = millis();
    }
    delay(10);
  }
}