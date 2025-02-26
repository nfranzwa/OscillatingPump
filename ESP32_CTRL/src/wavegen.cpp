#include <wavegen.h>
#include <Arduino.h>
#include "sharedData.h"

void WaveGenerator::begin() {
    // pinMode(PWM_PIN,OUTPUT);
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    // ledcAttachPin(PWM_PIN, PWM_CHANNEL);
    lastCycleStart = millis();
}

int WaveGenerator::generatePWM() {
    unsigned long current_time = millis() - lastCycleStart;
    int CYCLE_TIME = WaveGenerator::getPeriod();
    current_time = current_time % CYCLE_TIME;// might be unnecessary
    // basically a rising triangle wave from 0 to CYCLE_TIME
    if (current_time < ASDR[0]) {
        return map(current_time, 0, ASDR[0], PWM_min, PWM_max);
    }
    else if (current_time < (ASDR[0] + ASDR[1])) {
        return PWM_max;
    }
    else if (current_time < (ASDR[0] + ASDR[1] + ASDR[2])) {
        return map(current_time - (ASDR[0] + ASDR[1]), 
                  0, ASDR[2], PWM_max, PWM_min);
    }
    else {
        return PWM_min;
    }
}

//update the parameters of the waveform
void WaveGenerator::updateParams(int* new_asdr,int pwm_min, int pwm_max) {
    /*
    For safety, we have a copy of ASDR and PWM range on the wave generator.
    This is kept separate from the memory address manipulated by the user.
    */
    // ASDR=new_asdr; //this copies the mem address. bad.
    // memcpy copies the values stored in the mem-addresses
    // don't need to worry for ASDR, as it doesn't store any ptrs
    memcpy(ASDR,new_asdr,4*sizeof(int));
    PWM_min=pwm_min;
    PWM_max=pwm_max;
    // sharedData.cyc_period= WaveGenerator::getPeriod();
}

void WaveGenerator::update(int* ADSR,int min_PWM,int max_PWM,bool debug) {
    /* this function is run every LOOP_DELAY ms.
    In each instance of time, we output the corresponding PWM 
    for the correct timing of each phase.
    However, we want to update the wave params only once per period.
    */
    unsigned long current_time=millis();
    //start of new cycle
    if(current_time-lastCycleStart >= getPeriod()){
        lastCycleStart=current_time;
        bool valid=true;
        for(int i=0; i<4; i++){
            valid &=(ASDR[i]>=100);
        }
        if (valid) updateParams(ADSR,min_PWM,max_PWM);
    }
    // sharedData.PWM_value = generatePWM();
    if (debug) Serial.printf("PWM:%d\n",sharedData.PWM_value);
    // ledcWrite(PWM_CHANNEL, PWM_value);
}

void TF_wavegen(void *pvParameters) {
    WaveGenerator* wave = (WaveGenerator*) pvParameters;
    for (;;) {
        // TODO: change this to be using the PMAP value for the min and max pressures from calibration
        wave->update(sharedData.ASDR, sharedData.PWM_c_min, sharedData.PWM_c_max);
        sharedData.PWM_value=wave->generatePWM();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}