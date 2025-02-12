#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>
#include <esp_now.h>


const char* ssid = "Asian Crew";
const char* password = "Agastulate";

uint8_t broadcastAddress[] = {0x3c, 0x8a, 0x1f, 0xa8, 0xf9, 0x34};
float pressure;
float oscfreq;
float susttime;

/* float incomingpres;
float incomingoscfreq;
float incomingsusttime; */

AsyncWebServer server(80);
AsyncEventSource events("/events");

String sliderValue = "0";
String success;
const char* PARAM_INPUT = "value";
unsigned long lastTime = 0;
unsigned long timerDelay = 750;

String processor(const String& var){
  //Serial.println(var);
  if (var == "SLIDERVALUE"){
    return sliderValue;
  }
  return String();
}
typedef struct struct_message{
  float pres;
  float osc;
  float sust;
}struct_message;

/* struct_message fromsensors; */
struct_message tosensors;

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

/* void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&fromsensors, incomingData, sizeof(fromsensors));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingpres = fromsensors.pres;
  incomingoscfreq = fromsensors.osc;
  incomingsusttime = fromsensors.sust;
} */

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
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi ..");
  //Serial.println(cmH20)
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  Serial.println();
  initLittleFS();
  if (esp_now_init() != ESP_OK){
    Serial.println("Error with ESP Now");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr,broadcastAddress,6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  /* esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv)); */

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/pressureslider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sliderValue = inputMessage;
    }
    else {
      inputMessage = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "sample";
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
  float presslider = sliderValue.toFloat();
  tosensors.pres = presslider;
  tosensors.osc = 2;
  tosensors.sust = 4;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &tosensors, sizeof(tosensors));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  //Serial.println(fromsensors.pres);
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 10 seconds
    events.send("ping",NULL,millis());
    events.send(sliderValue.c_str(),"pressure",millis());
    lastTime = millis();
  }
 // printstuff();
  delay(1000);
} 

void printstuff(){
  Serial.println("INCOMING READINGS");
  Serial.print("Pressure: ");
  Serial.print(fromsensors.pres);
  Serial.print("Oscillation: ");
  Serial.print(fromsensors.osc);
  Serial.print("Sustain: ");
  Serial.print(fromsensors.sust);
}
