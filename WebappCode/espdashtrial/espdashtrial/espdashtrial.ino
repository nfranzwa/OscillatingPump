#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <LittleFS.h>

#define ZERO_SAMPLES 50
#define COUNTS_PER_PSI 735990  // Increased to further scale down PSI values
#define PSI_TO_CMH2O 70.307    // Conversion factor from PSI to cmH2O
#define DRIFT_THRESHOLD 0.1    // PSI threshold to consider as "no pressure"
#define STABLE_TIME 3000       // Time in ms that readings need to be stable for re-zeroing
#define DRIFT_SAMPLES 10 

int sliderval = 0;
const char* ssid = "";
const char* pw = "";
AsyncWebServer server(80);
AsyncEventSource events("/events");
ESPDash dashboard(&server);

int newone = 0;
long  zeroOffset = 0;
bool isCalibrated = false;
int sampleCount = 0;
long calibrationSum = 0;
// Drift compensation variables
unsigned long stableStartTime = 0;
unsigned long StartTime = millis();
bool isStable = false;
float lastPSI = 0;
int stableSampleCount = 0;
const int inputpin = 15;
const int outputpin = 2;  
int triggered = 0;

int ct = 0;
int counter_button = 0;
bool sevcount = false;
IPAddress local_IP(192, 168, 1, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 0, 0);

Card pressuredisplay(&dashboard, GENERIC_CARD, "Generic1", "cmH20");
Chart barchart(&dashboard, BAR_CHART,"Pressure (cmH20)");
Card startrec(&dashboard,BUTTON_CARD,"Start Recording");
Card pressureslider(&dashboard,SLIDER_CARD,"Pressure Slider","cmH20",0,40,1);
Card oscillationslider(&dashboard,SLIDER_CARD,"Oscillation Frequency","Hz",0,20);
Card cuffvolumeset(&dashboard,SLIDER_CARD,"Cuff Volume","cc",0,50,5);
Card sustaintime(&dashboard,SLIDER_CARD,"Sustain Time","s",0,10);
float YAxis[] ={0,0,0,0,0,0,0};
float XAxis[] = {0,0,0,0,0,0,0};

unsigned long lastTime = 0;
unsigned long timerDelay = 750;

void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
    Serial.println("LittleFS mounted successfully");
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pw);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  };
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

float readPressureRaw() {
  
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
    if (sampleCount >= 15){
      float zeroOffset = calibrationSum/15;
      isCalibrated = true;
      
    }
    return 0;
  } else {
    float psi = convertToPSI(pressure , zeroOffset);
    float cmH20 = psi * 70.307;
    checkAndUpdateZero(pressure,psi);
    return cmH20;
  }
};

void writeToCSV(float pressure, float timeStamp,int newfile) {
    
    if (newfile == 1){   
      File file = LittleFS.open("/data.csv", "w");
      file.printf("%.2f,%.2f\n", timeStamp, pressure);
      file.close();
    }else if (newfile == 0){
      File file = LittleFS.open("/data.csv", "a");
      file.printf("%.2f,%.2f\n", timeStamp, pressure);
      file.close();
    }   
}

void triggerDownload() {
    File file = LittleFS.open("/data.csv", "r");
    if (!file) {
        Serial.println("Failed to open CSV file");
        return;
    }
    file.close();
    server.on("/trigger", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", "<script>window.location.href='/download';</script>");
    });
}

void setup() {
  int ct = 0;
  // put your setup code here, to run once:
  pinMode(inputpin, INPUT);   // HX710 OUT
  pinMode(outputpin, OUTPUT);
  Serial.begin(115200);
  initWiFi();
  Serial.println();
  initLittleFS();
  digitalWrite(outputpin, LOW);
  for(int i = 0; i < 10; i++) {
    digitalWrite(outputpin, HIGH);

    digitalWrite(outputpin, LOW);
  }
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/data.csv", "text/csv");
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
  barchart.updateY(YAxis,7);
  barchart.updateX(XAxis,7);
  
  server.begin();

  /* Connect WiFi */
  /* WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, pw);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP()); */
 
  /* Start AsyncWebServer */
}

void loop() {
  // put your main code here, to run repeatedly:
  float pressuredata = readPressureRaw();
  unsigned long CurrentTime = millis();
  unsigned long ElapsedTime = CurrentTime - StartTime;
  if(ct == 8){
    for(int i = 1;i<=7;i++){
      YAxis[i-1] = YAxis[i];
      XAxis[i-1] = i; 
    }
    YAxis[7] = pressuredata;
    XAxis[7] = 7;
    barchart.updateY(YAxis,7);
    barchart.updateX(XAxis,7);
  }else{
    YAxis[ct] = pressuredata;
    XAxis[ct] = ct;
    ct +=1;
  }
  pressuredisplay.update(pressuredata);
  float timeStamp = millis()/1000.0;
  
  startrec.attachCallback([&](int value){
    /* startrec card updater - you need to update the startrec with latest value upon firing of callback */
    if(value == 1){
      triggered = 1;
      newone = 1;
    }else{
      triggered = 2;
    };
    startrec.update(value);
    /* Send update to dashboard */
    dashboard.sendUpdates();
  });
  if(triggered == 1){
    writeToCSV(pressuredata,ElapsedTime,newone);
  }else if (triggered == 2){
    triggerDownload();
  };
  newone = 0;
  pressureslider.attachCallback([&](float value){
    sliderval = value;
    server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request) {
        String valstring = String(sliderval,3);
        request->send(200, "application/json",valstring);
    });  
    pressureslider.update(value);
    dashboard.sendUpdates();
  });
  oscillationslider.attachCallback([&](float value){
    sliderval = value;
    server.on("/frequency", HTTP_GET, [](AsyncWebServerRequest *request) {
        String valstring = String(sliderval,3);
        request->send(200, "application/json",valstring);
    });  
    oscillationslider.update(value);
    dashboard.sendUpdates();
  });
  cuffvolumeset.attachCallback([&](float value){
    sliderval = value;
    server.on("/cuffvolume", HTTP_GET, [](AsyncWebServerRequest *request) {
        String valstring = String(sliderval,3);
        request->send(200, "application/json",valstring);
    });  
    cuffvolumeset.update(value);
    dashboard.sendUpdates();
  });
  sustaintime.attachCallback([&](float value){
    sliderval = value;
    server.on("/sustaintime", HTTP_GET, [](AsyncWebServerRequest *request) {
        String valstring = String(sliderval,3);
        request->send(200, "application/json",valstring);
    });  
    sustaintime.update(value);
    dashboard.sendUpdates();
  });
  dashboard.sendUpdates();
  delay(500);
}
