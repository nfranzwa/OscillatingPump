#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Wire.h>


#define TXD1 18
#define RXD1 19

#define ZERO_SAMPLES 50
#define COUNTS_PER_PSI 735990  // Increased to further scale down PSI values
#define PSI_TO_CMH2O 70.307    // Conversion factor from PSI to cmH2O
#define DRIFT_THRESHOLD 0.1    // PSI threshold to consider as "no pressure"
#define STABLE_TIME 3000       // Time in ms that readings need to be stable for re-zeroing
#define DRIFT_SAMPLES 10       // Number of samples to check for drift

float maptopsi(int value){
  if (value < -16383) value = -16383;
  if (value > 16383)  value = 16383;
  // Perform the mapping:
  // We need to scale from a 32766 range (-16383 to 16383) to a 200 range (-100 to 100)
  float result = (value / 16383.0) * 100.0;
  Serial.println(result);
  result = result * 0.0145;
  return result;
}
//HardwareSerial Serial2(2);
HardwareSerial mySerial(1);

float webpressure;
float oscfreq;
float susttime;

int lcdColumns = 20;
int lcdRows = 4;
//LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
String success;
long zeroOffset = 8290303;
bool isCalibrated = false;
int sampleCount = 0;
long calibrationSum = 0;
int counterforcalib = 0;
// Drift compensation variables
unsigned long stableStartTime = 0;
bool isStable = false;
float lastPSI = 0;
int stableSampleCount = 0;
const int inputpin = 15;
const int outputpin = 2;


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
  pinMode(inputpin, INPUT);
  pinMode(outputpin, OUTPUT);
  Serial.begin(115200);
  Wire.begin();
  Wire.beginTransmission(I2C_ADDRESS);
  if (Wire.endTransmission() == 0) {
    Serial.println("Pressure sensor found at 0x28.");
  } else {
    Serial.println("Pressure sensor not found! Check wiring.");
  }
  mySerial.begin(115200, SERIAL_8N1, RXD1, TXD1);
  WiFi.mode(WIFI_STA);
  digitalWrite(outputpin, LOW);
  delay(100);
  for (int i = 0; i < 10; i++) {
    digitalWrite(outputpin, HIGH);

    digitalWrite(outputpin, LOW);
  }
  /* lcd.init();
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
  lcd.print("s"); */
}

void loop() {
  String pressuredata = readPressureRaw();
  float presslider = pressuredata.toFloat();
  float pressuresensor = round(presslider * 100) / 100;
  Serial.println(pressuredata);
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
    /* lcd.setCursor(0, 2);
    lcd.print(pressuresensor);
    lcd.setCursor(10, 2);
    lcd.print(oscfreq);
    lcd.setCursor(16, 2);
    lcd.print(susttime); */
  }

  mySerial.println(pressuresensor);
  delay(100);
}
