#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>

#define TXD1 16
#define RXD1 17

// Use Serial1 for UART communication

//HardwareSerial Serial2(1);
HardwareSerial mySerial(2);


const char *ssid = "ESP32";
const char *password = "ucsdpumpguest";

float pressure;
float oscfreq;
float susttime;

int ct = 0;
AsyncWebServer server(80);
AsyncEventSource events("/events");

String presVal = "0";
String oscVal = "0";
String sustVal = "0";
String recording = "0";

String success;

const char *PARAM_INPUT = "value";
unsigned long lastTime = 0;
unsigned long timerDelay = 750;
int newfile = 0;
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
  WiFi.softAP(ssid,password); 
  delay(100);
  IPAddress Ip(192,168,1,85);
  IPAddress NMask(255,255,255,0);
  WiFi.softAPConfig(Ip,Ip,NMask);
  /*
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //Serial.println(cmH20)
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  */
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

  server.on("/Pressure", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      presVal = inputMessage;
    } else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/Oscillation", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      oscVal = inputMessage;
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
    /* String json = inputMessage;
    request->send_P(200, "text/plain",json.c_str());
    json = String(); */
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

  float PresValfloat = presVal.toFloat();
  float OscValfloat = oscVal.toFloat();
  float SustValfloat = sustVal.toFloat();



/*   if (mySerial.available()) {
 */    // Read data and display it
    //String message = mySerial.readStringUntil('\n');
    String message = "45";
    //Serial.println("Received: " + message);
    sensorpres = message;
    server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request) {
      String json =  sensorpres + "," + oscVal + "," + sustVal;
      request->send_P(200, "text/plain", json.c_str());
      json = String();
    });
    if (recording == "1") {
      if (seconditer == false) {
        newfile = 1;
      } else {
        newfile = 0;
      }
      unsigned currentTime = millis();
      seconditer = true;
    } else {
      seconditer = false;
    };

    if ((millis() - lastTime) > timerDelay) {
      // Send Events to the client with the Sensor Readings Every 10 seconds
      events.send("ping", NULL, millis());
      events.send(sensorpres.c_str(), "pressure", millis());
      lastTime = millis();
    }
  //}
  mySerial.print(PresValfloat);
  mySerial.print(", ");
  mySerial.print(OscValfloat);
  mySerial.print(", ");
  mySerial.println(SustValfloat);
  delay(100);
}
