#include <Arduino.h>
#include <physicalUI.h>
#include <Sensor.h>
#include <wavegen.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
// #include <WiFi.h>
#include "sharedData.h"

bool debug      = true; // output to COM port
bool useSensor1 = true;
bool useSensor2 = false;
bool useWave    = true;
bool useLCD     = true;

#define CLK         26
#define DT          27
#define SW          14
#define P_PIN       12 // pressure sensor output pin
#define P_PIN_NEW   34 // new pressure sensor tested
#define P_PIN_OUT   2  // pin to output pressure reading to

#define V_SUPPLY    3.3
#define V_MIN       0.1
#define V_MAX       3.08
#define P_MIN       0.0
#define P_MAX       10.0 // max pressure sensor output
#define ADC_RES     4095

#define SCL         36 // S(erial) CL(ock): Display
#define SDA         33 // S(erial) DA(ta):  Display

#define PWM_PIN     33 // PWM output pin
#define TXD1        18 // ESP serial communication
#define RXD1        19

// Constants from Agasthya's UI
#define ZERO_SAMPLES 50
#define COUNTS_PER_PSI 735990  // Increased to further scale down PSI values
#define PSI_TO_CMH2O 70.307    // Conversion factor from PSI to cmH2O
#define DRIFT_THRESHOLD 0.1    // PSI threshold to consider as "no pressure"
#define STABLE_TIME 3000       // Time in ms that readings need to be stable for re-zeroing
#define DRIFT_SAMPLES 10       // Number of samples to check for drift

HardwareSerial mySerial(1);
// IDK why Agasthya had this line or left it commented, but it's harmless
// HardwareSerial mySerial(2);

// todo: get rid of redundant variables, move to shared struct
SharedData sharedData;
String opt_name[] = {"Attack", "Sustain", "Decay", "Rest"};
int ASDR[4] = {100,200,300,400};
// String opt_name[] = {"1","2","3","4"};
static constexpr unsigned int LOOP_DELAY=3; // ms

//task handles
TaskHandle_t TH_sensor, TH_wavegen, TH_lcd, TH_ui=nullptr;
// task funct prototypes
void TF_sensor(void *pvParams);
void TF_wavegen(void *pvParams);
void TF_ui(void *pvParams);
void TF_lcd(void *pvParams);

LiquidCrystal_I2C lcd(0x27,20,4);  // set LCD_address: 0x27, num cols: 20, num rows: 4
PhysicalUI ui(CLK,DT,SW);
PSensor sensor1(P_PIN,"NXP", V_SUPPLY, V_MIN, V_MAX, P_MAX,P_MIN, ADC_RES); 
PSensor sensor2(P_PIN_NEW,"ABP",V_SUPPLY,V_MIN,V_MAX,P_MAX,P_MIN, ADC_RES);
WaveGenerator wave(PWM_PIN,sharedData.PWM_min,sharedData.PWM_max);

void setup() {
    Serial.begin(115200);
    mySerial.begin(115200,SERIAL_8N1,RXD1,TXD1);
    // WiFi.mode(WIFI_STA);
    pinMode(P_PIN_OUT,OUTPUT); // physical pin out for pressure

    sharedData.PWM_min = 0;
    sharedData.PWM_max = 4095;
    sharedData.PWM_value= 200;
    memcpy(sharedData.ASDR,ASDR,4*sizeof(int));
    ui.begin();
    ui.setOptions(sharedData.ASDR, opt_name, 100);
    sensor1.begin();
    wave.begin();
    // wave.updateParams(ASDR,sharedData.PWM_min,sharedData.PWM_max);
    lcd.init();
    lcd.backlight();
    lcd.setBacklight(128);
    ui.setLCD(&lcd);
    
    // create tasks
    xTaskCreatePinnedToCore(TF_sensor   ,"Sensor Task"  ,4000, &sensor1, 1, &TH_sensor , 0);
    xTaskCreatePinnedToCore(TF_wavegen  ,"Wavegen Task" ,4000, &wave   , 2, &TH_wavegen, 0);
    xTaskCreatePinnedToCore(TF_lcd      ,"LCD Task"     ,4000, &lcd    , 1, &TH_lcd    , 1);
    xTaskCreatePinnedToCore(TF_ui       ,"UI Task"      ,4000, &ui     , 1, &TH_ui     , 0);
}

void loop() {
    //FreeRTOS handles task scheduling so this can remain empty
    /*
    Serial.printf("PWM_min:%-4d\tPWM_max:%-4d\tPWM_value:%-4d\n",
        sharedData.PWM_min,sharedData.PWM_max,sharedData.PWM_value
    );
    */
    vTaskDelay(pdMS_TO_TICKS(10));
}