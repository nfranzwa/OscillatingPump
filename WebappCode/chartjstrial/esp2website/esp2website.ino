#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>
#include <LiquidCrystal_I2C.h>

#define ZERO_SAMPLES 50
#define COUNTS_PER_PSI 735990  // Increased to further scale down PSI values
#define PSI_TO_CMH2O 70.307    // Conversion factor from PSI to cmH2O
#define DRIFT_THRESHOLD 0.1    // PSI threshold to consider as "no pressure"
#define STABLE_TIME 3000       // Time in ms that readings need to be stable for re-zeroing
#define DRIFT_SAMPLES 10 
// Replace with your network credentials
int lcdColumns = 20;
int lcdRows = 4;
const char* ssid = "Asian Crew";
const char* password = "Agastulate";
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
char pressuredata[40];
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  
// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 750;

// GPIO where the DS18B20 sensors are connected to

// Setup a oneWire instance to communicate with OneWire devices (DS18B20)

// Pass our oneWire reference to Dallas Temperature sensor


// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
    Serial.println("LittleFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi ..");
  //Serial.println(cmH20)
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
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
    checkAndUpdateZero(pressure,psi);
    String pressurern = String(cmH20,3);
    return pressurern;
  }
};
  //String jsonresult = JSON.stringify(readings);
  //return jsonresult;
  //Serial.println(zeroOffset);
  
  
 

  
void setup() {
  pinMode(inputpin,INPUT);
  pinMode(outputpin, OUTPUT);
  // Serial port for debugging purposes
  Serial.begin(115200);
  initWiFi();
  Serial.println();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  // print message
  lcd.print("Press. |");
  // clears the display to print new message
  // set cursor to first column, second row
  lcd.setCursor(8,0);
  lcd.print("Oscil.|");
  lcd.setCursor(15,0);
  lcd.print("Sust.");
  lcd.setCursor(7, 1);
  lcd.print("|");
  lcd.setCursor(7,2);
  lcd.print("|");
  lcd.setCursor(7,3);
  lcd.print("|");
  lcd.setCursor(14, 1);
  lcd.print("|");
  lcd.setCursor(14,2);
  lcd.print("|");
  lcd.setCursor(14,3);
  lcd.print("|");
  lcd.setCursor(1, 3);
  lcd.print("cmH20");
  lcd.setCursor(10, 3);
  lcd.print("Hz");
  lcd.setCursor(17,3);
  lcd.print("s");
  /* // Setting the ESP as an access point
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP); */
  initLittleFS();
  digitalWrite(outputpin, LOW);
  for(int i = 0; i < 10; i++) {
    digitalWrite(outputpin, HIGH);

    digitalWrite(outputpin, LOW);
  }
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = readPressureRaw();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // Start server
  server.begin();

  
}

void loop() {
  String pressuredata = readPressureRaw();
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 10 seconds
    events.send("ping",NULL,millis());
    events.send(pressuredata.c_str(),"pressure",millis());
    lastTime = millis();
  }
  lcd.setCursor(0, 2);
  lcd.print(pressuredata);
  lcd.setCursor(10, 2);
  lcd.print("45");
  lcd.setCursor(16, 2);
  lcd.print("32");
}
