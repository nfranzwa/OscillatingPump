#include <LiquidCrystal_I2C.h>
#include <WiFi.h>


#define TXD1 18
#define RXD1 19
#define TXD2 34
#define RXD2 35
#define ZERO_SAMPLES 50
#define COUNTS_PER_PSI 735990  // Increased to further scale down PSI values
#define PSI_TO_CMH2O 70.307    // Conversion factor from PSI to cmH2O
#define DRIFT_THRESHOLD 0.1    // PSI threshold to consider as "no pressure"
#define STABLE_TIME 3000       // Time in ms that readings need to be stable for re-zeroing
#define DRIFT_SAMPLES 10       // Number of samples to check for drift



//HardwareSerial Serial2(2);
HardwareSerial mySerial(1);

float webpressure;
float oscfreq;
float susttime;

int lcdColumns = 20;
int lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
String success;
long zeroOffset = 8290303;
bool isCalibrated = false;
int sampleCount = 0;
long calibrationSum = 0;

// Drift compensation variables
unsigned long stableStartTime = 0;
bool isStable = false;
float lastPSI = 0;
int stableSampleCount = 0;
const int inputpin = 15;
const int outputpin = 2;


void checkAndUpdateZero(long rawPressure, float currentPSI) {
  // Check if current pressure is close to zero
  if (abs(currentPSI - 1) < DRIFT_THRESHOLD) {  // Remember we're applying -1 offset to PSI
    if (!isStable) {
      // Start tracking stable period
      isStable = true;
      stableStartTime = millis();
      stableSampleCount = 1;
    } else {
      stableSampleCount++;

      // Check if we've been stable long enough and have enough samples
      if (stableSampleCount >= DRIFT_SAMPLES && (millis() - stableStartTime) >= STABLE_TIME) {
        // Update zero offset
        zeroOffset = rawPressure;
        stableSampleCount = 0;
        isStable = false;
      }
    }
  } else {
    // Reset stable tracking if pressure is not near zero
    isStable = false;
    stableSampleCount = 0;
  }
}
float convertToPSI(long rawValue, long zero) {
  return (float)(rawValue - zero) / COUNTS_PER_PSI;
}

String readPressureRaw() {

  while (digitalRead(inputpin)) {}

  long result = 0;
  noInterrupts();

  for (int i = 0; i < 24; i++) {
    digitalWrite(outputpin, HIGH);
    digitalWrite(outputpin, LOW);
    result = (result << 1);
    if (digitalRead(inputpin)) {
      result++;
    }
  }

  for (char i = 0; i < 3; i++) {
    digitalWrite(outputpin, HIGH);
    digitalWrite(outputpin, LOW);
  }

  interrupts();

  float pressure = result ^ 0x800000;
  if (!isCalibrated) {
    calibrationSum += pressure;
    sampleCount++;
    if (sampleCount >= 3) {
      float zeroOffset = calibrationSum / 3;
      isCalibrated = true;
    }
    return "";
  } else {
    float psi = convertToPSI(pressure, zeroOffset);
    float cmH20 = psi * 70.307;
    String pressurern = String(cmH20, 2);
    return pressurern;
  }
};

float convertToPSI(long rawValue) {
  return (float)(rawValue - zeroOffset) / COUNTS_PER_PSI;
}

float convertToCmH2O(float psi) {
  return psi * PSI_TO_CMH2O;
}


void setup() {
  pinMode(inputpin, INPUT);
  pinMode(outputpin, OUTPUT);
  Serial.begin(115200);
  mySerial.begin(115200, SERIAL_8N1, RXD1, TXD1);
  WiFi.mode(WIFI_STA);
  digitalWrite(outputpin, LOW);
  delay(100);
  for (int i = 0; i < 10; i++) {
    digitalWrite(outputpin, HIGH);

    digitalWrite(outputpin, LOW);
  }
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
  String pressuredata = readPressureRaw();
  float presslider = pressuredata.toFloat();
  float pressuresensor = round(presslider * 100) / 100;
  if (mySerial.available()) {
    // Read data and display it
    String message = mySerial.readStringUntil('\n');
    //Serial.println("Received: " + message);
    char inputArray[message.length() + 1];  // Create a char array of the correct size
    message.toCharArray(inputArray, sizeof(inputArray));
    char *token = strtok(inputArray, ",");
    if (token != NULL) webpressure = atoi(token);
    token = strtok(NULL, ",");
    if (token != NULL) oscfreq = atoi(token);
    token = strtok(NULL, ",");
    if (token != NULL) susttime = atoi(token);
    lcd.setCursor(0, 2);
    lcd.print(pressuresensor);
    lcd.setCursor(10, 2);
    lcd.print(oscfreq);
    lcd.setCursor(16, 2);
    lcd.print(susttime);
  }
  
  mySerial.println(pressuresensor);
  delay(100);
}
