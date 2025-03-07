#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>

#define TXD1 16
#define RXD1 17

// Use Serial1 for UART communication

HardwareSerial mySerial(2);

/*
const char *ssid = "ESP32";
const char *password = "ucsdpumpguest";
*/
const char *ssid = "Asian Crew";
const char *password = "Agastulate";

String calibrationstate = "0";

float pressure;
float attacktimefloat = 0;
float SustValfloat;
float resttimefloat;

AsyncWebServer server(80);
AsyncEventSource events("/events");

String minpresVal = "0";
String maxpresVal = "0";
String attacktime = "0";
String sustVal = "0";
String recording = "0";
String resttime = "0";
String desiredpos = "0";

String success;

const char *PARAM_INPUT = "value";
unsigned long lastTime = 0;
unsigned long timerDelay = 750;
bool seconditer = false;

String sensorpres = "0";

void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  } else {
    Serial.println("LittleFS mounted successfully");
  }
}

void initWiFi() {
  /*
  WiFi.softAP(ssid, password);
  delay(100);
  IPAddress Ip(192, 168, 1, 85);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);
  */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //Serial.println(cmH20)
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}


void setup() {
  Serial.begin(115200);
  initWiFi();
  Serial.println();
  initLittleFS();
  mySerial.begin(115200, SERIAL_8N1, RXD1, TXD1);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");

  server.on("/MinPressure", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      minpresVal = inputMessage;
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

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
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", recording);
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

  float MinPresValfloat = minpresVal.toFloat();
  float attacktimefloat = attacktime.toFloat();
  float SustValfloat = sustVal.toFloat();
  float MaxPresValfloat = maxpresVal.toFloat();
  float resttimefloat = resttime.toFloat();
  int calibrationfloat = calibrationstate.toInt();
  int desiredposfloat = desiredpos.toInt();

  if (mySerial.available()) {
    // Read data and display it
    String message = mySerial.readStringUntil('\n');
    Serial.println("Received: " + message);
    char inputArray[message.length() + 1];
    message.toCharArray(inputArray, sizeof(inputArray));
    char *token = strtok(inputArray, ",");
    if (token != NULL) sensorpres = atoi(token);
    token = strtok(NULL, ",");
    if (token != NULL) calibrationstate = atoi(token); 
    server.on("/readings", HTTP_GET, [=](AsyncWebServerRequest *request) {
      String json = sensorpres + "," + String(attacktimefloat) + "," + String(SustValfloat) + "," + String(resttimefloat);
      request->send_P(200, "text/plain", json.c_str());
      json = String();
    });
    if ((millis() - lastTime) > timerDelay) {
      // Send Events to the client with the Sensor Readings Every 10 seconds
      events.send("ping", NULL, millis());
      events.send(sensorpres.c_str(), "pressure", millis());
      events.send(calibrationstate.c_str(), "calstate", millis());
      lastTime = millis();
    }
  }


  mySerial.print(MinPresValfloat);
  mySerial.print(",");
  mySerial.print(MaxPresValfloat);
  mySerial.print(",");
  mySerial.print(SustValfloat);
  mySerial.print(",");
  mySerial.print(resttimefloat);
  mySerial.print(",");
  mySerial.print(attacktimefloat);
  mySerial.print(",");
  mySerial.print(calibrationfloat);
  mySerial.print(",");
  mySerial.print(desiredposfloat);
  mySerial.print("\n");
  delay(100);
}
