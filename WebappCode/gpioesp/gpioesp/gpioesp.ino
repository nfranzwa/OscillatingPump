#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <MightyZap.h>

#define I2C_ADDRESS 0x28
#define ID_NUM 0
#define MIN_POS 100
#define MAX_POS 4095  // Default max position
#define TXD1 18
#define RXD1 19
#define MIGHTY_ZAP_RX 16  // Define RX pin for Serial2
#define MIGHTY_ZAP_TX 17  // Define TX pin for Serial2
#define MIGHTY_ZAP_EN 13  // Enable pin

HardwareSerial mySerial(1);

uint8_t data[7];
uint8_t cmd[3] = { 0xAA, 0x00, 0x00 };
double press_counts = 0;
double pressure = 0;
double outputmax = 15099494;
double outputmin = 1677722;
double pmax = 1;
double pmin = 0;
float minpressure;
float maxpressure;
float attacktime;
float resttime;
int calibrationstate;
float desiredposition;
float susttime;

int lcdColumns = 20;
int lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
int sustaintime = 1000;
float frequency = 0.75;            // Default frequency in Hz
unsigned long lastSensorRead = 0;  // For timing sensor readings
unsigned long cycleStartTime = 0;  // For tracking cycle timing
const float DUTY_CYCLE = 0.4;

MightyZap m_zap(&Serial2, MIGHTY_ZAP_EN, 1);

int calibration(int calibrationstate) {
  Serial.println("Calibrating");
  calibrationstate = 1;
  int calstarttime = 0;
  int reached = 0;
  while (calibrationstate == 1) {
    Serial.printf("MODEL NUM:%d\n", m_zap.getModelNumber(ID_NUM));
    int curpos = m_zap.presentPosition(ID_NUM);
    if (reached == 0) {
      if (abs(curpos - MIN_POS) < 3) {
        reached = 1;
        int calstarttime = millis();
      } else {
        int err = MIN_POS - curpos;
        if (abs(curpos - MIN_POS) > 10) {
          m_zap.GoalPosition(ID_NUM, MIN_POS);
        } else {
          m_zap.GoalPosition(ID_NUM, MIN_POS + err);
        }
      }
    } else {
      if (abs(curpos - MAX_POS) < 3) {
        calibrationstate = 2;
        Serial.println("done");
        Serial.println(calibrationstate);
        return calibrationstate;
      } else {
        if (millis() - calstarttime > 2000) {
          m_zap.GoalPosition(ID_NUM, MAX_POS);
        }
      }
    }
  }
}


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
  int distance = MAX_POS - MIN_POS;
  int speed = calculateSpeed(distance, frequency);
  m_zap.GoalSpeed(ID_NUM, speed);
  Serial.printf("Updated speed to: %d\n", speed);
}

String readPressureRaw() {
  Wire.beginTransmission(I2C_ADDRESS);
  int stat = Wire.write(cmd, 3);
  stat |= Wire.endTransmission();
  delay(10);
  int i = 0;
  Wire.requestFrom(I2C_ADDRESS, 7);
  for (i = 0; i < 7; i++) {
    data[i] = Wire.read();
  }
  press_counts = data[3] + data[2] * 256 + data[1] * 65536;  // calculate digital pressure counts
  pressure = ((press_counts - outputmin) * (pmax - pmin)) / (outputmax - outputmin) + pmin;
  String psi = String(pressure, 3);

  return psi;
}

void setup() {

  Serial.begin(115200);
  Wire.begin();
  Serial2.begin(32, SERIAL_8N1, MIGHTY_ZAP_RX, MIGHTY_ZAP_TX);
  m_zap.begin(32);
  Wire.beginTransmission(I2C_ADDRESS);
  mySerial.begin(115200, SERIAL_8N1, RXD1, TXD1);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Press. |");
  lcd.setCursor(8, 0);
  lcd.print("Oscil.|");
  lcd.setCursor(15, 0);
  lcd.print("Sust.");
  lcd.setCursor(7, 1);
  lcd.print("|");
  lcd.setCursor(7, 2);
  lcd.print("|");
  lcd.setCursor(7, 3);
  lcd.print("|");
  lcd.setCursor(14, 1);
  lcd.print("|");
  lcd.setCursor(14, 2);
  lcd.print("|");
  lcd.setCursor(14, 3);
  lcd.print("|");
  lcd.setCursor(1, 3);
  lcd.print("cmH20");
  lcd.setCursor(10, 3);
  lcd.print("Hz");
  lcd.setCursor(17, 3);
  lcd.print("s"); 
}

void loop() {
  String pres = readPressureRaw();
  float totalPeriod = 1000.0 / frequency;             // Period in milliseconds
  float activeTime = totalPeriod * DUTY_CYCLE;        // Active time in milliseconds
  float restTime = totalPeriod * (1.0 - DUTY_CYCLE);  // Rest time in milliseconds
  unsigned long currentTime = millis() - cycleStartTime;

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

  /* if (calibrationstate == 0) {
    calibrationstate = calibration(calibrationstate);
  } */

  if (currentTime < activeTime) {
    // First half of active period: forward movement
    if (currentTime < activeTime / 2) {
      m_zap.GoalPosition(ID_NUM, 2000);
    }
    // Second half of active period: backward movement
    else {
      m_zap.GoalPosition(ID_NUM, 4095);
      calibrationstate = calibration(calibrationstate);

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
  while (m_zap.Moving(ID_NUM)) {
    if (millis() - lastSensorRead >= 25) {
      lastSensorRead = millis();
    }
    delay(10);
  }
  mySerial.print("42");
  mySerial.print(",");
  mySerial.println(calibrationstate);
  delay(100);
}
