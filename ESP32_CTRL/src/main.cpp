#include <Arduino.h>
#include <Rotaryencoder.h>
#include <Sensor.h>
#include <wavegen.h>

//TODO: Move pin definitions here
#define CLK         26
#define DT          27
#define SW          14
#define P_PIN       12 // pressure sensor output pin
#define V_SUPPLY    3.3
#define V_MIN       0.1
#define V_MAX       3.08
#define P_MAX       10.0 // max pressure sensor output
#define ADC_RES     4095
#define PWM_PIN     33 // PWM output pin
int PWM_MIN=        0;
int PWM_MAX=        4095;

int PWM_VAL;
int ASDR[4] = {250,250,250,250};
String opt_name[] = {"Attack", "Sustain", "Decay", "Rest"};

static constexpr unsigned int LOOP_DELAY=3;//ms

RotaryEncoder encoder(CLK,DT,SW);
PSensor sensor(P_PIN, V_SUPPLY, V_MIN, V_MAX, P_MAX, ADC_RES); 

WaveGenerator wave(PWM_PIN,PWM_MIN,PWM_MAX);

bool debug=true; // output to serial port
bool useSensor= false;
bool useWave  = true;
void setup() {
    Serial.begin(115200);
    encoder.begin();
    encoder.setOptions(ASDR, opt_name, 100);
    if(useSensor) sensor.begin();
    if(useWave) {
        wave.begin();
        wave.updateParams(ASDR,PWM_MIN,PWM_MAX);
    }
}

void loop() {
    encoder.update();
    Serial.printf("%s: ", encoder.isEditMode() ? "EDIT" : "VIEW");
    Serial.printf("Variable: %-2s\tValue: %6d ms\t",
        encoder.getCurrentName().c_str(),
        encoder.getCurrentValue());
    if (useSensor){
        float p_raw=sensor.readPressure();
        float p_filter=sensor.filter(p_raw);
        // sensor.sensor_debug(debug);
        // TODO: digitalWrite to a pin to send to other ESP
        if(debug) Serial.printf("P_raw: %-8.2fkPa\tP_filter: %-8.2fkPa\t",p_raw,p_filter);
    }
    if(useWave) {
        wave.update(ASDR,PWM_MIN,PWM_MAX);
        if(debug) Serial.printf("PWM: %-6d\t",wave.PWM_value);
    }
    Serial.printf("\n");
    delay(LOOP_DELAY);
}