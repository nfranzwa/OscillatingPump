#include <wavegen.h>
#include <Arduino.h>
#include "sharedData.h"
// #include <sensor.h>

int mapPos(float P_target);
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
        if(sharedData.P_current<=sharedData.P_min){
            Serial.println("updating last_min");
            sharedData.PWM_last_min=sharedData.PWM_value;
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
    Serial.println("Start Waveform task");
    for (;;) {
        //calculate PWM_last_min;
        /*
        the PWM_last_min is creeping upwards from leaks
        when we update the P_min to a lower or higher
        if position of syringe for new desired position
        is less than the PWM_last_min, update the last_min 
        */
        // Serial.println("waveform loop");
        if(sharedData.calibration_state==2){
            if(mapPos(sharedData.P_min)<sharedData.PWM_last_min){
                // so if we want to move further back
                Serial.printf("target below current P_min");
                sharedData.PWM_last_min=mapPos(sharedData.P_min);
            }
            // then set PWM_last_min to be the map from pmap
            //calculate PWM_offset
            sharedData.PWM_offset=mapPos(sharedData.P_max)-mapPos(sharedData.P_min);
            if(sharedData.PWM_offset+sharedData.PWM_last_min>4095){
                sharedData.calibration_state=4;
                Serial.println("movement would be out of range");
            }
            else{ // safe range of values;
                Serial.println("Updating waveform max, min");
                //use pwm_last_min, offset value calculated
                sharedData.PWM_min=sharedData.PWM_last_min;
                sharedData.PWM_max=sharedData.PWM_last_min+sharedData.PWM_offset;
            }
            Serial.printf("Floor:%d\tOffset:%d\n",sharedData.PWM_last_min,sharedData.PWM_offset);
            //update function does the handshake to update wave object's values.
            wave->update(sharedData.ASDR, sharedData.PWM_min, sharedData.PWM_max);
            sharedData.PWM_value=wave->generatePWM();
        }
        vTaskDelay(pdMS_TO_TICKS(7));
    }
}

// as long as the target pressure is within the calibration range, run a binary search
// WARNING: binary search assumes the values sorted (aka monotonic)
int mapPos(float P_target) {
    // Ensure target pressure is within the calibration range
    if (P_target > sharedData.P_max || P_target < sharedData.P_min) {
        Serial.println("Target pressure out of range, recalibrate");
        return sharedData.PWM_c_min;
    }

    // Directly return limits if exactly matching
    if (P_target == sharedData.P_max) return sharedData.PWM_c_max;
    if (P_target == sharedData.P_min) return sharedData.PWM_c_min;

    int left = sharedData.PWM_c_min;
    int right = sharedData.PWM_c_max;

    // Ensure valid range
    if (left >= right) {
        Serial.println("Invalid calibration range, check PWM limits");
        return left;
    }

    int closestPos = left;
    float minDiff = fabs(sharedData.pmap[left] - P_target);
    
    int i = 0; // Debugging counter
    while (left <= right) {
        int mid = left + (right - left) / 2;

        // Ensure mid is within valid range
        if (mid < sharedData.PWM_c_min || mid > sharedData.PWM_c_max) {
            Serial.println("Binary search index out of bounds, aborting");
            return closestPos;
        }

        float midValue = sharedData.pmap[mid];
        float diff = fabs(midValue - P_target);

        // Update closest position
        if (diff < minDiff) {
            minDiff = diff;
            closestPos = mid;
        }

        // Adjust search range
        if (midValue < P_target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }

        // Debugging loop limit
        i++;
        if (i > 12) {
            Serial.println("Binary search error, too many loops. Returning closest guess");
            return closestPos;
        }
    }
    Serial.printf("T_P:%f\tP_min:%f\tP_max%f\tT_PWM:%f\n",P_target,sharedData.P_min,sharedData.P_max,closestPos);
    return closestPos;
}
