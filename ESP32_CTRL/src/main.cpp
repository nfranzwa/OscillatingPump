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
#include "esp_int_wdt.h"
#include <algorithm>
bool debug      = false; // output to COM port
bool useSensor1 = true;
bool useSensor2 = false;
bool useWave    = true;
bool useLCD     = true;

SharedData sharedData;
String opt_name[] = {"Attack", "Sustain", "Decay", "Rest"};
int ASDR[4] = {1000,2000,100,1500};

// String opt_name[] = {"1","2","3","4"};

//task handles
TaskHandle_t TH_sensor, TH_wavegen, TH_lcd, TH_ui, TH_statusLED, TH_motor=nullptr;
TaskHandle_t TH_calibrate,TH_talk2web, TH_ptest=nullptr;
// task funct prototypes
void TF_sensor(void *pvParams);
void TF_wavegen(void *pvParams);
void TF_ui(void *pvParams);
void TF_lcd(void *pvParams);
void TF_motor(void *pvParams);
void TF_calibrate(void *pvParams);
void TF_talk2web(void *pvParams);
void TF_ptest(void *pvParams);
void TF_status_LED(void *pvParams);

// HardwareSerial Serial2(2);
HardwareSerial mySerial(1);
MightyZap m_zap(&Serial2, MIGHTY_ZAP_EN, 1);

LiquidCrystal_I2C lcd(0x27,20,4);  // set LCD_address: 0x27, num cols: 20, num rows: 4
PhysicalUI ui(CLK,DT,SW);
// PSensor sensor1(P_PIN,"NXP", V_SUPPLY, V_MIN, V_MAX, P_MAX,P_MIN, ADC_RES); 
// PSensor sensor2(P_PIN_NEW,"ABP",V_SUPPLY,V_MIN,V_MAX,P_MAX,P_MIN, ADC_RES);
PSensor sensor1(P_PIN,"I2C",V_SUPPLY,V_MIN,V_MAX,P_MAX,P_MIN, ADC_RES);
WaveGenerator wave(sharedData.PWM_min,sharedData.PWM_max);
MotorControl motor(MIGHTY_ZAP_RX,MIGHTY_ZAP_TX,MIGHTY_ZAP_EN);

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, MIGHTY_ZAP_RX, MIGHTY_ZAP_TX);
    Wire.begin();
    mySerial.begin(115200, SERIAL_8N1,RXD1,TXD1);
    // pinMode(P_PIN_OUT,OUTPUT); // physical pin out for pressure
    // moved to sensor.cpp 
    // Wire.beginTransmission(I2C_ADDRESS);
    // if(Wire.endTransmission()==0) Serial.println("Pressure sensor found at 0x28");
    // else Serial.println("sensor not found, check wiring");
    // digitalWrite(P_PIN_OUT,LOW);
    sharedData.pmap.fill(-1.0);
    sharedData.calibration_state=0;
    sharedData.PWM_min = 0;
    sharedData.PWM_max = 4095;
    sharedData.PWM_value= 200;
    memcpy(sharedData.ASDR,ASDR,4*sizeof(int));
    // test values for wave generation w/o calibration
    sharedData.PWM_c_min=000;
    sharedData.PWM_c_max=3000;
    sharedData.P_min=0.05;
    sharedData.P_max=1.0;
    for (int i =sharedData.PWM_c_min=000; i <= sharedData.PWM_c_max; i++){
        sharedData.pmap[i] = (i - sharedData.PWM_c_min) * (0.6f / (sharedData.PWM_c_max - sharedData.PWM_c_min));
    }
    ui.begin();
    ui.setOptions(sharedData.ASDR, opt_name, 100);
    sensor1.begin();
    motor.setMotor(&m_zap);
    motor.begin();
    // wave.updateParams(ASDR,sharedData.PWM_min,sharedData.PWM_max);
    lcd.init();
    lcd.backlight();
    lcd.setBacklight(128);
    ui.setLCD(&lcd);
    // create tasks
    xTaskCreatePinnedToCore(TF_sensor   ,"Sensor Task"  ,4000   , &sensor1, 2, &TH_sensor , 0);
    xTaskCreatePinnedToCore(TF_talk2web ,"Web Comm Task",100000 , nullptr , 3, &TH_talk2web,0);
    xTaskCreatePinnedToCore(TF_wavegen  ,"Wavegen Task" ,6000   , &wave   , 1, &TH_wavegen, 0);
    xTaskCreatePinnedToCore(TF_lcd      ,"LCD Task"     ,5000   , &lcd    , 1, &TH_lcd    , 1);
    xTaskCreatePinnedToCore(TF_ui       ,"UI Task"      ,6000   , &ui     , 1, &TH_ui     , 0);
    // xTaskCreatePinnedToCore(TF_status_LED,"statLED Task",4000   , nullptr , 0, &TH_statusLED,0);
    xTaskCreatePinnedToCore(TF_motor    ,"Motor Task"   ,4000   , &motor  , 0, &TH_motor  ,0);
    xTaskCreatePinnedToCore(TF_calibrate,"Calib. Task"  ,8000   , &motor  , 0, &TH_calibrate,0);
    // xTaskCreatePinnedToCore(TF_ptest    ,"testP Task"   ,2000   , nullptr , 0, &TH_ptest  ,0);

}

void loop() {
    //FreeRTOS handles task scheduling so this can remain empty
    String serialInput;
    float value;
    if(Serial.available()){
        serialInput=Serial.readStringUntil('\n');
        //if there's a new message that matches, rewrite the variables
        if(serialInput.startsWith("debug")){
            debug=!debug;
            Serial.printf("Updated: debug=%d",debug);
            // Consider adding the rest of the debug toggles here
        }
        if (sscanf(serialInput.c_str(), "mp:%f", &value) == 1) {
            // Successfully parsed the message
            Serial.printf("Updated: test = %-4.4f\n",value);
            Serial.printf("mapPos(%2.2f)=%d\n",(float) value,mapPos(value));
        }
        if (sscanf(serialInput.c_str(),"c:%d",&sharedData.calibration_state)){
            Serial.printf("MANUAL: Calibration state %d\n",sharedData.calibration_state);
        }
        if (serialInput.equals("wave debug")){
            sharedData.wave_debug=!sharedData.wave_debug;
            Serial.printf("Updated: wave debug=%d",sharedData.wave_debug);
        }
        if (serialInput.equals("cal debug")){
            sharedData.cal_debug=!sharedData.cal_debug;
            Serial.printf("Updated: cal debug=%d",sharedData.cal_debug);
        }
        if (serialInput.equals("show pm")){
            int i = 0;
            for (const float& value : sharedData.pmap) {
                Serial.printf("Index %-4d: %+2.3f\n", i, value);
                i++;
            }
        }
        if(sscanf(serialInput.c_str(),"m:%f M:%f",&sharedData.P_min,&sharedData.P_max)){
            Serial.printf("Updated: Pm=%2.2f, PM=%2.2f\n",sharedData.P_min,sharedData.P_max);
        }
    }
    //otherwise continue changing the sharedData.P_test as normal
    delay(100);
    // delay(100);
}

void TF_talk2web(void* pvParams){
    // if theres a message sent to the serial port
    Serial.println("Start T2W Task");
    for(;;){
        if(mySerial.available()){
            sharedData.prev_state=sharedData.calibration_state;
            // Serial.println("start reading");
            vTaskDelay(pdMS_TO_TICKS(10));
            String msg = mySerial.readStringUntil('\n');
            // String msg="help";
            if(debug) Serial.println("Received "+msg);
            //if the new message matches format, rewrite the variables
            //MinP, MaxP, S, R, A/D, calibration_state, manual position
            
            if (sscanf(msg.c_str(), "%f,%f,%d,%d,%d,%d,%d",
                &sharedData.P_minH2O, &sharedData.P_maxH2O,&sharedData.ASDR[1],
                &sharedData.ASDR[3],&sharedData.ASDR[0],
                &sharedData.calibration_state,&sharedData.PWM_manual) == 7
                ) {
                sharedData.ASDR[2]= sharedData.ASDR[0];
                if(sharedData.calibration_state==1 && sharedData.prev_state==2){
                    sharedData.calibration_state=2;
                    if(debug) Serial.println("Received 1, I was 2");
                }
                if(sharedData.calibration_state==3 && sharedData.prev_state==2){
                    sharedData.calibration_state=3;
                }
                //convert input (H2O) to psi
                sharedData.P_min=sharedData.P_minH2O/70.307;
                sharedData.P_max=sharedData.P_maxH2O/70.307;
                // Successfully parsed the message
                //print statement for update
                if(debug){
                    Serial.printf("Pmin%-2.2f Pmax%-2.2f A/D:%d S:%d R%d\n",
                        sharedData.P_min,sharedData.P_max,sharedData.ASDR[0],sharedData.ASDR[1],sharedData.ASDR[3]);    
                }
            }
        }
        if(debug){
            Serial.printf("%f,%d\n",sharedData.P_current ,sharedData.calibration_state);
        }
        mySerial.printf("%f,%d\n",sharedData.P_current*70.0307,sharedData.calibration_state);
        // Serial.println("T2W end of loop");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}