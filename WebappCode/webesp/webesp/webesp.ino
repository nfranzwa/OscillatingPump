#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>

#define TXD1 16
#define RXD1 17

const int greenLedPin = 4;  // Main LED
const int redLedPin = 0;
// Use Serial1 for UART communication
// Add these variables to track changes
String prev_minpresVal = "0";
String prev_maxpresVal = "20";
String prev_attacktime = "500";
String prev_sustVal = "1200";
String prev_resttime = "1200";
String prev_desiredpos = "0";
String prev_calibrationstate = "0";

// Last time data was sent to peripheral
unsigned long lastPeripheralUpdate = 0;
const long peripheralUpdateInterval = 100;  // 100ms between serial sends

HardwareSerial mySerial(2);

const char *ssid = "ESP32";
const char *password = "ucsdpumpguest";
/*
const char *ssid = "Asian Crew";
const char *password = "Agastulate";
*/
String calibrationstate = "0";
int calint = 0;  // Global variable to track calibration state for LED status

float pressure = 0.0;
float attacktimefloat = 0;
float SustValfloat = 0;
float resttimefloat = 0;
float currentSensorPressure = 0.0;  // Global variable to store current pressure

unsigned long previousMillis = 0;
const long blinkInterval = 500;  // Blink interval in milliseconds (0.5 second)
int greenLedState = LOW;
int redLedState = LOW;

AsyncWebServer server(80);
AsyncEventSource events("/events");

String minpresVal = "0";
String maxpresVal = "20";
String attacktime = "500";
String sustVal = "1200";
String recording = "0";
String resttime = "1200";
String desiredpos = "0";

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
  IPAddress Ip(192, 168, 1, 85);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);
  /*
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //Serial.println(cmH20)
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  */
  Serial.println(WiFi.localIP());
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

// Handler for readings endpoint - moved outside of loop
void handleReadings(AsyncWebServerRequest *request) {
  char sensorpresString[15];  // Buffer to hold the string
  dtostrf(currentSensorPressure, 1, 6, sensorpresString);

  int attacktimeInt = attacktime.toInt();
  int sustValInt = sustVal.toInt();
  int resttimeInt = resttime.toInt();

  // Convert char array to String before concatenation
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

  server.on("/MinPressure", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      minpresVal = inputMessage;

      // Immediately send the updated value to peripheral
      sendToPeripheral();
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  // Apply similar changes to the other parameter handlers

  server.on("/MaxPressure", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      maxpresVal = inputMessage;
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/AttackTime", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      attacktime = inputMessage;
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/position", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      desiredpos = inputMessage;
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/record", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
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
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      resttime = inputMessage;
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/calibration", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      calibrationstate = inputMessage;
      Serial.println(calibrationstate);
      if (calibrationstate != "2") {
        int overwrite = 1;
      } else {
        int overwrite = 0;
      }
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", calibrationstate);
  });

  server.on("/SustainTime", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sustVal = inputMessage;
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  events.onConnect([](AsyncEventSourceClient *client) {
    if (client->lastId()) {
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

  unsigned long currentMillis = millis();
  calint = calibrationstate.toInt();
  int tempholder = calibrationstate.toInt();

  /* int MinPresValfloat = minpresVal.toInt();
  int attacktimeInt = attacktime.toInt();
  int SustValInt = sustVal.toInt();
  int MaxPresValfloat = maxpresVal.toInt();
  int resttimeInt = resttime.toInt();
  int calint = calibrationstate.toInt();
  int desiredposfloat = desiredpos.toInt();
  unsigned long currentMillis = millis();
  int tempholder = calibrationstate.toInt(); */


  if (calint == 1) {
    // STATE 1: Blink the green LED
    digitalWrite(redLedPin, LOW);  // Make sure red LED is off

    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;

      // Toggle green LED state
      greenLedState = (greenLedState == LOW) ? HIGH : LOW;
      digitalWrite(greenLedPin, greenLedState);
    }
  } else if (calint == 2 || calint == 3) {
    // STATE 2 or 3: Green LED stays on
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(redLedPin, LOW);
  } else if (calint == 4) {
    // STATE 4: Red LED on, Green LED off
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
  } else if (calint == 5) {
    // STATE 5: Blink the red LED
    digitalWrite(greenLedPin, LOW);  // Make sure green LED is off

    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;

      // Toggle red LED state
      redLedState = (redLedState == LOW) ? HIGH : LOW;
      digitalWrite(redLedPin, redLedState);
    }
  } else {
    // Default state: both LEDs off
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, LOW);
  }

  if (mySerial.available()) {
    // Read data and display it
    String message = mySerial.readStringUntil('\n');
    Serial.println("Received: " + message);

    float sensorpres = 0.00000;  // Initialize with a default value
    int calibrstate = 0;

    // Make sure the message isn't empty before processing
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
    }

    // Send events if it's time


    if (currentMillis - lastPeripheralUpdate >= peripheralUpdateInterval) {
      lastPeripheralUpdate = currentMillis;

      // Only send if any values have changed
      if (minpresVal != prev_minpresVal || maxpresVal != prev_maxpresVal || attacktime != prev_attacktime || sustVal != prev_sustVal || resttime != prev_resttime || desiredpos != prev_desiredpos || calibrationstate != prev_calibrationstate) {

        sendToPeripheral();
      }
    }

    if ((currentMillis-lastTime) > timerDelay) {
        // Send Events to the client with the Sensor Readings
        events.send("ping", NULL, millis());
        events.send(String(currentSensorPressure).c_str(), "pressure", millis());
        events.send(String(calibrationstate).c_str(), "calstate", millis());
        lastTime = millis();
      }
  }




  // Send data to the peripheral device
  /* mySerial.print(MinPresValfloat);
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
  mySerial.print(",");
  mySerial.print(desiredposfloat);
  mySerial.print("\n");
  delay(100); */
}

void sendToPeripheral() {
  int MinPresValfloat = minpresVal.toInt();
  int attacktimeInt = attacktime.toInt();
  int SustValInt = sustVal.toInt();
  int MaxPresValfloat = maxpresVal.toInt();
  int resttimeInt = resttime.toInt();
  // Use the global calint variable instead of local declaration
  calint = calibrationstate.toInt();  // Update the global variable
  int desiredposfloat = desiredpos.toInt();

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
  mySerial.print(",");
  mySerial.print(desiredposfloat);
  mySerial.print("\n");

  // Update previous values
  prev_minpresVal = minpresVal;
  prev_maxpresVal = maxpresVal;
  prev_attacktime = attacktime;
  prev_sustVal = sustVal;
  prev_resttime = resttime;
  prev_desiredpos = desiredpos;
  prev_calibrationstate = calibrationstate;
}