#include <Arduino.h>
#include <physicalUI.h>
#include <Sensor.h>
#include <wavegen.h>
#include <motorcontrol.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "sharedData.h"
#include <MightyZap.h>

bool debug      = true; // output to COM port
bool useSensor1 = true;
bool useSensor2 = false;
bool useWave    = true;
bool useLCD     = true;

SharedData sharedData;
String opt_name[] = {"Attack", "Sustain", "Decay", "Rest"};
int ASDR[4] = {1500,8000,200,500};
// String opt_name[] = {"1","2","3","4"};

//task handles
TaskHandle_t TH_sensor, TH_wavegen, TH_lcd, TH_ui, TH_motor,TH_calibrate,TH_talk2Web=nullptr;
// task funct prototypes
void TF_sensor(void *pvParams);
void TF_wavegen(void *pvParams);
void TF_ui(void *pvParams);
void TF_lcd(void *pvParams);
void TF_motor(void *pvParams);
void TF_calibrate(void *pvParams);

// HardwareSerial Serial2(2);
HardwareSerial SerialWeb(1);
MightyZap m_zap(&Serial2, MIGHTY_ZAP_EN, 1);

LiquidCrystal_I2C lcd(0x27,20,4);  // set LCD_address: 0x27, num cols: 20, num rows: 4
PhysicalUI ui(CLK,DT,SW);
PSensor sensor1(P_PIN,"NXP", V_SUPPLY, V_MIN, V_MAX, P_MAX,P_MIN, ADC_RES); 
PSensor sensor2(P_PIN_NEW,"ABP",V_SUPPLY,V_MIN,V_MAX,P_MAX,P_MIN, ADC_RES);
PSensor sensor3(P_PIN,"I2C",V_SUPPLY,V_MIN,V_MAX,P_MAX,P_MIN, ADC_RES);
WaveGenerator wave(sharedData.PWM_min,sharedData.PWM_max);
MotorControl motor(MIGHTY_ZAP_RX,MIGHTY_ZAP_TX,MIGHTY_ZAP_EN);

void setup() {
    Serial.begin(115200);
    Serial2.begin(32, SERIAL_8N1, MIGHTY_ZAP_RX, MIGHTY_ZAP_TX);
    SerialWeb.begin(32, SERIAL_8N1,RXD1,TXD1);
    Wire.begin();
    
    WiFi.mode(WIFI_STA);
    
    pinMode(P_PIN_OUT,OUTPUT); // physical pin out for pressure
    Wire.beginTransmission(I2C_ADDRESS);
    if(Wire.endTransmission()==0) Serial.println("Pressure sensor found at 0x28");
    else Serial.println("sensor not found, check wiring");
    digitalWrite(P_PIN_OUT,LOW);
    sharedData.PWM_min = 100;
    sharedData.PWM_max = 4095;
    sharedData.PWM_value= 200;
    delay(100);
    for (int i = 0; i < 10; i++) {
        digitalWrite(P_PIN_OUT, HIGH);
        digitalWrite(P_PIN_OUT, LOW);
      }
    
    memcpy(sharedData.ASDR,ASDR,4*sizeof(int));
    ui.begin();
    ui.setOptions(sharedData.ASDR, opt_name, 100);
    sensor1.begin();
    wave.begin();
    motor.setMotor(&m_zap);
    motor.begin();
    // wave.updateParams(ASDR,sharedData.PWM_min,sharedData.PWM_max);
    lcd.init();
    lcd.backlight();
    lcd.setBacklight(128);
    ui.setLCD(&lcd);
    
    // create tasks
    xTaskCreatePinnedToCore(TF_sensor   ,"Sensor Task"  ,4000, &sensor1, 1, &TH_sensor , 0);
    xTaskCreatePinnedToCore(TF_lcd      ,"LCD Task"     ,4000, &lcd    , 1, &TH_lcd    , 1);
    xTaskCreatePinnedToCore(TF_wavegen  ,"Wavegen Task" ,4000, &wave   , 2, &TH_wavegen, 0);
    xTaskCreatePinnedToCore(TF_ui       ,"UI Task"      ,4000, &ui     , 1, &TH_ui     , 0);
    //choose whether we want to update pressure map continuously or not;
    // we can't guarantee that regular usage will have monotonic pressure at the sensor position
    xTaskCreatePinnedToCore(TF_talk2Web ,"Web comm Task",4000, nullptr , 1, &TH_talk2Web,0);
    
    xTaskCreatePinnedToCore(TF_calibrate,"Calibrate Task",4000,&motor  , 0, &TH_calibrate,0);
    //TODO: Add user input to trigger calibration
    xTaskCreatePinnedToCore(TF_motor    ,"Motor Task"   ,4000, &motor  , 0, &TH_motor   , 0);
}

void loop() {
    //FreeRTOS handles task scheduling so this can remain empty
    /*
    Serial.printf("PWM_min:%-4d\tPWM_max:%-4d\tPWM_value:%-4d\n",
        sharedData.PWM_min,sharedData.PWM_max,sharedData.PWM_value
    );
    */
    //Serial.printf("%f\n",(float) millis());
    if(SerialWeb.available()){
        String msg = SerialWeb.readStringUntil('\n');
        Serial.println("Received "+msg);
        char inputArray[msg.length() + 1];  // Create a char array of the correct size
        msg.toCharArray(inputArray, sizeof(inputArray));
        char *token = strtok(inputArray, ",");
        if (token != NULL) sharedData.P_min = atoi(token);
        token = strtok(NULL, ",");
        if (token != NULL) sharedData.P_max = atoi(token);
        token = strtok(NULL, ",");
        // Sustain
        if (token != NULL) sharedData.ASDR[1] = atoi(token);
        token = strtok(NULL, ",");
        // Rest
        if (token != NULL) sharedData.ASDR[3] = atoi(token);
        token = strtok(NULL, ",");
        // Attack and Decay
        if (token != NULL){
            sharedData.ASDR[0]= atoi(token);
            sharedData.ASDR[2]= sharedData.ASDR[0];
        }
        if (token != NULL) sharedData.calibration_state = atoi(token);
        token = strtok(NULL, ",");
        if (token != NULL) sharedData.manualPWM = atoi(token);
        token = strtok(NULL, ",");
        
        Serial.printf("Pmin%-2.2f Pmax%-2.2f A/D:%d S:%d R%d\n",
            sharedData.P_min,sharedData.P_max,sharedData.ASDR[0],sharedData.ASDR[1],sharedData.ASDR[3]);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
}

void TF_talk2Web (void* pvParams){
    SerialWeb.printf("%2.1f,%d\n",sharedData.P_current,sharedData.calibration_state);
    vTaskDelay(pdMS_TO_TICKS(100));
}