#include <wavegen.h>
#include <Arduino.h>
#include "sharedData.h"
#include <sensor.h>
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
        //update PWM_last_min 
        if(sharedData.P_current>=sharedData.P_min){
            sharedData.PWM_last_min=sharedData.PWM_current;
        }
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

/* this function is run every LOOP_DELAY ms.
In each instance of time, we output the corresponding PWM 
for the correct timing of each phase.
However, we want to update the wave params only once per period.
*/
void WaveGenerator::update(int* ADSR,int min_PWM,int max_PWM,bool debug) {
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
    if (debug) Serial.printf("PWM:%d\n",sharedData.PWM_value);
}
/*
Update the generated PWM value:
- PWM_min, PWM_max are the ones sent to waveform generated
- PWM_c_min, PWM_c_max: range to search in pmap
use 
- P_min, P_max: to calculate PWM_min, PWM_max
- PWM_last_min: feedback on last highest position below min pressure
- PWM_offset: data from calibration (pmap) for offset
DO:
- calculate what to set PWM_min, PWM_max as
    - use pwm_last_min
    - calculate offset from pmap using P_min, P_max

- change calibration state if target range is out of bounds

*/
void TF_wavegen(void *pvParameters) {
    WaveGenerator* wave = (WaveGenerator*) pvParameters;
    for (;;) {
        //calculate PWM_last_min;
        /*
        the PWM_last_min is creeping upwards from leaks
        when we update the P_min to a lower or higher
        if position of syringe for new desired position
        is less than the PWM_last_min, update the last_min 
        */
        if(mapPos(sharedData.P_min)<sharedData.PWM_last_min){
            // so if we want to move further back
            sharedData.PWM_last_min=mapPos(sharedData.P_min);
        }
        // then set PWM_last_min to be the map from pmap
        //calculate PWM_offset
        // PWM_offset=pmap of P_max - pmap of P_min
        if(sharedData.PWM_offset+sharedData.PWM_last_min>4095){
            sharedData.calibration_state=4;
        }
        else{ // safe range of values;
            //use pwm_last_min, offset value calculated 
            sharedData.PWM_min=sharedData.PWM_last_min;
            sharedData.PWM_max=sharedData.PWM_last_min+sharedData.PWM_offset;
        }
        //update function does the handshake to update wave object's values.
        wave->update(sharedData.ASDR, sharedData.PWM_min, sharedData.PWM_max);
        sharedData.PWM_value=wave->generatePWM();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}