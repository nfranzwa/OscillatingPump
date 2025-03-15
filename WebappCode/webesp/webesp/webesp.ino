#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>

#define TXD1 19
#define RXD1 18

const int greenLedPin = 26;  // Main LED
const int redLedPin = 25;
// Use Serial1 for UART communication
// Add these variables to track changes
String prev_minpresVal = "0";
String prev_maxpresVal = "20";
String prev_attacktime = "500";
String prev_sustVal = "1200";
String prev_resttime = "1200";
String prev_calibrationstate = "0";

// Last time data was sent to peripheral
unsigned long lastPeripheralUpdate = 0;
const long peripheralUpdateInterval = 100;  // 100ms between serial sends

// Force periodic updates even without changes
unsigned long lastForcedUpdate = 0;
const long forcedUpdateInterval = 2000;  // 2 seconds

HardwareSerial mySerial(2);


const char *ssid = "ESP32";
const char *password = "ucsdpumpguest";

String calibrationstate = "0";
int calint = 0;  // Global variable to track calibration state for LED status
int errorcode = 0;
float pressure = 0.0;
float attacktimefloat = 0;
float SustValfloat = 0;
float resttimefloat = 0;
float currentSensorPressure = 0.0;  // Global variable to store current pressure

unsigned long previousMillis = 0;
const long blinkInterval = 500;  // Blink interval in milliseconds (0.5 second)
int greenLedState = LOW;
int redLedState = LOW;

// Flag to indicate web UI values have changed
bool valuesChanged = false;

AsyncWebServer server(80);
AsyncEventSource events("/events");

String minpresVal = "0";
String maxpresVal = "20";
String attacktime = "500";
String sustVal = "1200";
String recording = "0";
String resttime = "1200";

String success;

const char *PARAM_INPUT = "value";
unsigned long lastTime = 0;
unsigned long timerDelay = 60;
bool seconditer = false;


void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  } else {
    Serial.println("LittleFS mounted successfully");
  }
}

void initWiFi() {
  
  WiFi.softAP(ssid, password);
  delay(100);
  IPAddress IP(192, 168, 1, 85);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(IP, IP, NMask);
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

// Improved sendToPeripheral function with buffer clearing
void sendToPeripheral() {
  // Clear any pending data first
  while (mySerial.available()) {
    mySerial.read();  // Clear buffer
  }

  int MinPresValfloat = minpresVal.toInt();
  int attacktimeInt = attacktime.toInt();
  int SustValInt = sustVal.toInt();
  int MaxPresValfloat = maxpresVal.toInt();
  int resttimeInt = resttime.toInt();
  calint = calibrationstate.toInt();  // Update the global variable

  // Print debug message to main serial
  Serial.println("Sending to peripheral: " + String(MinPresValfloat) + "," + String(MaxPresValfloat) + "," + String(SustValInt) + "," + String(resttimeInt) + "," + String(attacktimeInt) + "," + String(calint));

  mySerial.print(MinPresValfloat);
  mySerial.print(",");
  mySerial.print(MaxPresValfloat);
  mySerial.print(",");
  mySerial.print(SustValInt);
  mySerial.print(",");
  mySerial.print(resttimeInt);
  mySerial.print(",");
  mySerial.print(attacktimeInt);
  mySerial.print(",");
  mySerial.print(calint);
  mySerial.print("\n");

  // Update previous values
  prev_minpresVal = minpresVal;
  prev_maxpresVal = maxpresVal;
  prev_attacktime = attacktime;
  prev_sustVal = sustVal;
  prev_resttime = resttime;
  prev_calibrationstate = calibrationstate;

  lastPeripheralUpdate = millis();
  valuesChanged = false;
}

// Handler for readings endpoint - moved outside of loop
void handleReadings(AsyncWebServerRequest *request) {
  char sensorpresString[15];
  dtostrf(currentSensorPressure, 1, 6, sensorpresString);
  int attacktimeInt = attacktime.toInt();
  int sustValInt = sustVal.toInt();
  int resttimeInt = resttime.toInt();
  String json = String(sensorpresString) + "," + String(attacktimeInt) + "," + String(sustValInt) + "," + String(resttimeInt);
  request->send(200, "text/plain", json.c_str());
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  Serial.println();
  initLittleFS();
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  mySerial.begin(115200, SERIAL_8N1, RXD1, TXD1);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");

  // Register the readings endpoint handler HERE in setup, not in loop
  server.on("/readings", HTTP_GET, handleReadings);

  // Modified handlers to send values immediately
  server.on("/MinPressure", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      minpresVal = inputMessage;
      valuesChanged = true;

      // Send immediately without waiting for interval
      sendToPeripheral();
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/MaxPressure", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      maxpresVal = inputMessage;
      valuesChanged = true;

      // Send immediately without waiting for interval
      sendToPeripheral();
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/AttackTime", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      attacktime = inputMessage;
      valuesChanged = true;

      // Send immediately without waiting for interval
      sendToPeripheral();
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/record", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      recording = inputMessage;
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", recording);
  });

  server.on("/RestTime", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      resttime = inputMessage;
      valuesChanged = true;

      // Send immediately without waiting for interval
      sendToPeripheral();
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/calibration", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      calibrationstate = inputMessage;
      valuesChanged = true;

      // Send immediately without waiting for interval
      sendToPeripheral();
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", calibrationstate);
  });

  server.on("/SustainTime", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sustVal = inputMessage;
      valuesChanged = true;

      // Send immediately without waiting for interval
      sendToPeripheral();
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  events.onConnect([](AsyncEventSourceClient *client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();

  // Send initial values to peripheral
  sendToPeripheral();
}

void loop() {
  unsigned long currentMillis = millis();
  calint = calibrationstate.toInt();
  int tempholder = calibrationstate.toInt();

  // Handle LED status based on calibration state
  if (calint == 1) {
    digitalWrite(redLedPin, LOW);
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      greenLedState = (greenLedState == LOW) ? HIGH : LOW;
      digitalWrite(greenLedPin, greenLedState);
    }
  } else if (calint == 2 || calint == 3) {
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(redLedPin, LOW);
  } else if (calint == 4 || calint == 0) {
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
  } else if (calint == 5) {
    digitalWrite(greenLedPin, LOW);
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      redLedState = (redLedState == LOW) ? HIGH : LOW;
      digitalWrite(redLedPin, redLedState);
    }
  } else {
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, LOW);
  }

  // Handle incoming messages from peripheral
  if (mySerial.available()) {
    String message = mySerial.readStringUntil('\n');
    Serial.println("Received: " + message);
    float sensorpres = 0.00000;
    int calibrstate = 0;
    String errorcode = "0";

    if (message.length() > 0) {
      char inputArray[message.length() + 1];
      message.toCharArray(inputArray, sizeof(inputArray));
      char *token = strtok(inputArray, ",");
      if (token != NULL) {
        sensorpres = atof(token);            // Assign value to the variable
        currentSensorPressure = sensorpres;  // Update global variable
      }
      token = strtok(NULL, ",");
      if (token != NULL) {
        calibrstate = atoi(token);
        if (calibrstate == 2) {
          calint = 2;
          calibrationstate = "2";
        }
        if (tempholder != calibrstate) {
          if (calibrstate == 2) {
            if (tempholder == 1) {
              calint = 2;
              calibrstate = 2;
              calibrationstate = "2";
              Serial.print("here");
            } else {
              calibrationstate = String(tempholder);
              calint = tempholder;
              calibrstate = tempholder;
            }
          }
        }
      }
      token = strtok(NULL, ",");
      if (token != NULL) {
        int receivedError = atoi(token);
        errorcode = String(receivedError);
      }
    }
  }

  // Forced periodic update regardless of changes
  if (currentMillis - lastForcedUpdate >= forcedUpdateInterval) {
    lastForcedUpdate = currentMillis;
    sendToPeripheral();
    Serial.println("Forced update sent to peripheral");
  }

  // Regular updates when values have changed
  if (valuesChanged && (currentMillis - lastPeripheralUpdate >= peripheralUpdateInterval)) {
    sendToPeripheral();
  }

  // Update web events
  if ((currentMillis - lastTime) > timerDelay) {
    events.send("ping", NULL, millis());
    events.send(String(currentSensorPressure).c_str(), "pressure", millis());
    //events.send(String(errorcode).c_str(), "errorcode", millis());
    String bo = "1";
    events.send(bo.c_str(),"errorcode",millis());
    events.send(String(calibrationstate).c_str(), "calstate", millis());
    lastTime = millis();
  }
}