#include <Arduino.h>
#include <physicalUI.h>
#include <Sensor.h>
#include <wavegen.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
// #include <WiFi.h>

//TODO: Move pin definitions here
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

int PWM_MIN=        0;
int PWM_MAX=        4095;

int PWM_VAL;
int ASDR[4] = {250,250,250,250};
// String opt_name[] = {"Attack", "Sustain", "Decay", "Rest"};
String opt_name[] = {"1","2","3","4"};

static constexpr unsigned int LOOP_DELAY=3; // ms
float pressure;

LiquidCrystal_I2C lcd(0x27,20,4);  // set LCD_address: 0x27, num cols: 20, num rows: 4
PhysicalUI ui(CLK,DT,SW);
PSensor sensor1(P_PIN,"NXP", V_SUPPLY, V_MIN, V_MAX, P_MAX,P_MIN, ADC_RES); 
PSensor sensor2(P_PIN_NEW,"ABP",V_SUPPLY,V_MIN,V_MAX,P_MAX,P_MIN, ADC_RES);
WaveGenerator wave(PWM_PIN,PWM_MIN,PWM_MAX);


bool debug      = true; // output to COM port
bool useSensor1 = true;
bool useSensor2 = false;
bool useWave    = true;
bool useLCD     = true;

void setup() {
    Serial.begin(115200);
    mySerial.begin(115200,SERIAL_8N1,RXD1,TXD1);
    // WiFi.mode(WIFI_STA);
    pinMode(P_PIN_OUT,OUTPUT); // physical pin out for pressure
    ui.begin();
    ui.setOptions(ASDR, opt_name, 100);
    if(useSensor1) sensor1.begin();
    if(useSensor2) sensor2.begin();
    if(useWave) {
        wave.begin();
        wave.updateParams(ASDR,PWM_MIN,PWM_MAX);
    }
    if(useLCD){
        lcd.init();
        lcd.backlight();
        ui.setLCD(&lcd);    
    }
}

void loop() {
    ui.update(debug);
    // TODO: Turn this all into the LCD output
    // Allow multiple modes to toggle through.
    if(debug){
        Serial.printf("%s: ", ui.getViewMode());
        Serial.printf("Variable: %-2s\tValue:%-6d ms\t",
            ui.getCurrentName().c_str(),
            ui.getCurrentValue());
    }
    if (useSensor1){
        float p_raw=sensor1.readPressure();
        float p_filter=sensor1.filter(p_raw);
        // sensor.sensor_debug(debug);
        // TODO: digitalWrite to a pin to send to other ESP
        if(debug) Serial.printf("P_raw: %-6.3fkPa\tP_filter: %-6.3fkPa\t",p_raw,p_filter);
        // digitalWrite(P_PIN_OUT,p_filter);
    }
    if (useSensor2){
        float p_raw=sensor2.readPressure();
        float p_filter=sensor2.filter(p_raw);
        // sensor.sensor_debug(debug);
        // TODO: digitalWrite to a pin to send to other ESP
        if(debug) Serial.printf("Sensor 2: P_raw: %-6.3fkPa\tP_filter: %-6.3fkPa\t",p_raw,p_filter);
        digitalWrite(P_PIN_OUT,p_filter);
    }
    if(useWave) {
        wave.update(ASDR,PWM_MIN,PWM_MAX);
        if(debug) Serial.printf("PWM: %-6d\t",wave.PWM_value);
    }
    if(debug) Serial.printf("\n");

    ui.updateLCD(sensor2.filter(sensor2.readPressure()),
        (float) wave.PWM_value);
}