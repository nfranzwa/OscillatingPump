#include <Wire.h>
#include <MightyZap.h>
#define ID_NUM 0
#define MIN_POS 100
#define TXD1 18
#define RXD1 19
#define MIGHTY_ZAP_RX 16  // Define RX pin for Serial2
#define MIGHTY_ZAP_TX 17  // Define TX pin for Serial2
#define MIGHTY_ZAP_EN 13  // Enable pin
#define ANALOG_PIN 34

MightyZap m_zap(&Serial2, MIGHTY_ZAP_EN, 1);

HardwareSerial mySerial(1);

int sustaintime = 1000;
int maxPosition = 2000;            // Default max position
float frequency = 0.75;            // Default frequency in Hz
unsigned long lastSensorRead = 0;  // For timing sensor readings
unsigned long cycleStartTime = 0;  // For tracking cycle timing
const float DUTY_CYCLE = 0.4;
float minpressure;
float maxpressure;
float attacktime;
float resttime;
float calibrationstate;
float desiredposition;
float susttime;

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


std::vector<uint16_t> myList = {};
void updateParameters() {
  int distance = maxPosition - MIN_POS;
  int speed = calculateSpeed(distance, frequency);
  m_zap.GoalSpeed(ID_NUM, speed);
  Serial.printf("Updated speed to: %d\n", speed);
}

#define I2C_ADDRESS 0x28
// I2C address of the pressure sensor
float maptopsi(int value) {
  if (value < -16383) value = -16383;
  if (value > 16383) value = 16383;
  // Perform the mapping:
  // We need to scale from a 32766 range (-16383 to 16383) to a 200 range (-100 to 100)
  float result = (value / 16383.0) * 100.0;
  result = result * 0.0145;
  return result;
}
String readPressureRaw() {
  int pressure_raw = 0;
  Wire.requestFrom(I2C_ADDRESS, 2);
  if (Wire.available() >= 2) {
    uint8_t highByte = Wire.read();
    uint8_t lowByte = Wire.read();
    pressure_raw = (highByte << 8) | lowByte;
  } else {
    Serial.println("No data received.");
  }
  float psi = maptopsi(pressure_raw);

  //Serial.println(psi);
  String pressure = String(psi, 3);

  return pressure;
}
void setup() {

  Serial.begin(115200);
  Wire.begin();  // Initialize I2C (SDA: GPIO21, SCL: GPIO22)
  Serial2.begin(32, SERIAL_8N1, MIGHTY_ZAP_RX, MIGHTY_ZAP_TX);
  m_zap.begin(32);
  mySerial.begin(115200, SERIAL_8N1, RXD1, TXD1);
  analogSetWidth(12);
  cycleStartTime = millis();
  // Check if the sensor is responding
  Wire.beginTransmission(I2C_ADDRESS);
  if (Wire.endTransmission() == 0) {
    Serial.println("Pressure sensor found at 0x28.");
  } else {
    Serial.println("Pressure sensor not found! Check wiring.");
  }
}

void loop() {

  String pres = readPressureRaw();
  Serial.println(pres);
  
  
  // Calculate cycle timing
  float totalPeriod = 1000.0 / frequency;             // Period in milliseconds
  float activeTime = totalPeriod * DUTY_CYCLE;        // Active time in milliseconds
  float restTime = totalPeriod * (1.0 - DUTY_CYCLE);  // Rest time in milliseconds
  unsigned long currentTime = millis() - cycleStartTime;

  // Reset cycle if needed
  if (currentTime >= totalPeriod) {
    cycleStartTime = millis();
    currentTime = 0;
  }
  
  if (mySerial.available()) {
    // Read data and display it
    String message = mySerial.readStringUntil('\n');
    Serial.println("Received: " + message);
    char inputArray[message.length() + 1];  // Create a char array of the correct size
    message.toCharArray(inputArray, sizeof(inputArray));
    char *token = strtok(inputArray, ",");
    if (token != NULL) minpressure = atoi(token);
    token = strtok(NULL, ",");
    if (token != NULL) maxpressure = atoi(token);
    token = strtok(NULL, ",");
    if (token != NULL) attacktime = atoi(token);
    token = strtok(NULL, ",");
    if (token != NULL) sustaintime = atoi(token);
    token = strtok(NULL, ",");
    if (token != NULL) resttime = atoi(token);
    token = strtok(NULL, ",");
    if (token != NULL) calibrationstate = atoi(token);
    token = strtok(NULL, ",");
    if (token != NULL) desiredposition = atoi(token);
  } else {
    Serial.println("Not coming");
  }
  
  // Active period (40% of cycle)
  if (currentTime < activeTime) {
    // First half of active period: forward movement
    if (currentTime < activeTime / 2) {
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
  }0

  // Continue sensor readings during movement
  while (m_zap.Moving(ID_NUM)) {
    if (millis() - lastSensorRead >= 25) {
      lastSensorRead = millis();
    }
    delay(10);
  }
  
  
  mySerial.print(pres);
  mySerial.print(",");
  mySerial.println(calibrationstate);

  delay(100);
}
