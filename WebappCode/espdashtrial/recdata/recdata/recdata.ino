/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>
#include <LiquidCrystal_I2C.h>
#include <ESPAsyncWebServer.h>



int lcdColumns = 20;
int lcdRows = 4;
const char *ssid = "Asian Crew";
const char *password = "Agastulate";
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String message = "";
String pressureslider = "0";

JSONVar sliderValues;

String getSliderValues() {
  sliderValues["pressureslider"] = String(pressureslider);
  String jsonString = JSON.stringify(sliderValues);
  return jsonString;
}
String processor(const String &var) {
  if (var == "SLIDERVALUE") {
    return sliderValue;
  }
  return String();
};

float pressval = 0;
float oscval = 0;
float sustval = 0;
int cuffvol = 0;

unsigned long lastTime = 0;
unsigned long timerDelay = 300;

String sensorReadings;
float sensorReadingsArr[3];

const char *PARAM_INPUT = "value";

void notifyClients(String sliderValues) {
  ws.textAll(sliderValues);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char *)data;
    if (message.indexOf("1s") >= 0) {
      pressureslider = message.substring(2);
      dutyCycle1 = map(sliderValue1.toInt(), 0, 100, 0, 255);
      Serial.println(dutyCycle1);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
    }
    if (strcmp((char *)data, "getValues") == 0) {
      notifyClients(getSliderValues());
    }
  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}
void setup() {

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi");
  }
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  // print message
  lcd.print("Press. |");
  // clears the display to print new message
  // set cursor to first column, second row
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

  initWebSocket();
  /* server.on("http://192.168.1.85/slider", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("here");
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sliderValue = inputMessage;
    } else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  }); */
  server.begin();
}

void loop() {
  ws.cleanupClients();
  /*
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      String serverPath = serverName + "/pressure?value=0";
      Serial.println(serverPath);
      // Your Domain name with URL path or IP address with path
      http.begin(serverPath.c_str());
      
      // If you need Node-RED/server authentication, insert user and password below
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {

        String payload = http.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
  */
}
