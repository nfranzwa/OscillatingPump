#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <esp_now.h>



#define ZERO_SAMPLES 50
#define COUNTS_PER_PSI 735990  // Increased to further scale down PSI values
#define PSI_TO_CMH2O 70.307    // Conversion factor from PSI to cmH2O
#define DRIFT_THRESHOLD 0.1    // PSI threshold to consider as "no pressure"
#define STABLE_TIME 3000       // Time in ms that readings need to be stable for re-zeroing
#define DRIFT_SAMPLES 10       // Number of samples to check for drift

uint8_t broadcastAddress[] = {0xf8, 0xb3, 0xb7, 0x4f, 0x4f, 0xf8};
/* float pressure;
float oscfreq;
float susttime; */

float incomingpres;
float incomingoscfreq;
float incomingsusttime;

int lcdColumns = 20;
int lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
String success;
long  zeroOffset = 8290303;
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

typedef struct struct_message{
  float pres;
  float osc;
  float sust;
}struct_message;

struct_message fromwebpage;
/* struct_message towebpage;
 */
esp_now_peer_info_t peerInfo;

/* void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}
 */
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&fromwebpage, incomingData, sizeof(fromwebpage));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingpres = fromwebpage.pres;
  incomingoscfreq = fromwebpage.osc;
  incomingsusttime = fromwebpage.sust;
}

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
      if (stableSampleCount >= DRIFT_SAMPLES && 
          (millis() - stableStartTime) >= STABLE_TIME) {
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
    digitalWrite(outputpin, LOW);  }
  
  interrupts();

  float pressure = result ^ 0x800000;
  if (!isCalibrated){
    calibrationSum += pressure;
    sampleCount++;
    if (sampleCount >= 3){
      float zeroOffset = calibrationSum/3;
      isCalibrated = true;
      
    }
    return "";
  } else {
    float psi = convertToPSI(pressure , zeroOffset);
    float cmH20 = psi * 70.307;
    String pressurern = String(cmH20,3);
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
  pinMode(inputpin,INPUT);
  pinMode(outputpin, OUTPUT);
  // Serial port for debugging purposes
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  //esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Initialize sensor - power cycle the clock line
  digitalWrite(outputpin, LOW);
  delay(100);
  for(int i = 0; i < 10; i++) {
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
  /* towebpage.pres = presslider;
  towebpage.osc = 6;
  towebpage.sust = 9; */

  /* esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &towebpage, sizeof(towebpage));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
 */

  //Serial.println(fromwebpage.pres);

  lcd.setCursor(0, 2);
  lcd.print(fromwebpage.pres);
  lcd.setCursor(10, 2);
  lcd.print("45");
  lcd.setCursor(16, 2);
  lcd.print("32");
  printstuff();
  delay(1000);
}

void printstuff(){
  Serial.println("INCOMING READINGS");
  Serial.print("Pressure: ");
  Serial.print(fromwebpage.pres);
  Serial.print("Oscillation: ");
  Serial.print(fromwebpage.osc);
  Serial.print("Sustain: ");
  Serial.print(fromwebpage.sust);
}