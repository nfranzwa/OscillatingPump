#include <wavegen.h>
#include <Arduino.h>
#include "sharedData.h"
// #include <sensor.h>

int WaveGenerator::generatePWM() {
    unsigned long current_time = millis() - lastCycleStart;
    int CYCLE_TIME = WaveGenerator::getPeriod();
    current_time = current_time % CYCLE_TIME;// might be unnecessary
    // basically a rising triangle wave from 0 to CYCLE_TIME
    if (current_time < ASDR[0]) {
        //essentially an interpolation
        if(transition="exp"){
            float progress = (float)current_time / ASDR[0];
            // Exponential curve for attack (eases in)
            return PWM_min + (1.0 - exp(-2.0*progress))/(1.0 - exp(-2.0)) * (PWM_max - PWM_min);
        }
        else return map(current_time, 0, ASDR[0], PWM_min, PWM_max);
    }
    else if (current_time < (ASDR[0] + ASDR[1])) {
        return PWM_max;
    }
    else if (current_time < (ASDR[0] + ASDR[1] + ASDR[2])) {
        if(transition="exp"){
            float progress = (float)(current_time - (ASDR[0] + ASDR[1]))/ ASDR[2];
            // Exponential curve for attack (eases in)
            return PWM_min + (exp(-2.0 * progress) - exp(-2.0))/(1.0 - exp(-2.0)) * (PWM_max - PWM_min);
        }
        else return map(current_time - (ASDR[0] + ASDR[1]), 0, ASDR[2], PWM_max, PWM_min);
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
    int prev_PWM;
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
            int mappedMin=mapPos(sharedData.P_min);
            int mappedMax=mapPos(sharedData.P_max);
            if(sharedData.wave_debug) Serial.printf("P_min: %2.3f, mappedMin: %d, PWM_last_min: %d\n", 
            (float) sharedData.P_min, (int) mappedMin, sharedData.PWM_last_min);
            
            // User lowers P_min
            if(mappedMin<sharedData.PWM_last_min){
                sharedData.PWM_last_min=mappedMin;
                if(sharedData.wave_debug) Serial.printf("P_min lowered: Updated PWM_last_min to %d\n", sharedData.PWM_last_min);
            }
                
            //calculate PWM_offset
            sharedData.PWM_offset=mappedMax-mappedMin;
            if(sharedData.PWM_offset+sharedData.PWM_last_min>4095){
                sharedData.calibration_state=4;
                sharedData.error=2;
                if(sharedData.wave_debug) Serial.println("movement would be out of range");
            }
            else{ // safe range of values;
                if(sharedData.wave_debug) Serial.println("Updating waveform max, min");
                //use pwm_last_min, offset value calculated
                sharedData.PWM_min=sharedData.PWM_last_min;
                sharedData.PWM_max=sharedData.PWM_last_min+sharedData.PWM_offset;
            }
            if(sharedData.wave_debug) Serial.printf("Floor:%d\tOffset:%d\n",sharedData.PWM_last_min,sharedData.PWM_offset);
            //update function does the handshake to update wave object's values.
            wave->update(sharedData.ASDR, sharedData.PWM_min, sharedData.PWM_max);
            int newPWM_value=wave->generatePWM();
            // could use sharedData.PWM_value instead, but not sure how it'd interact w/ rest of loop
            if(sharedData.P_current<sharedData.P_min
                && prev_PWM<newPWM_value){ 
                sharedData.PWM_last_min=sharedData.PWM_value;
            }
            prev_PWM=sharedData.PWM_value;
            sharedData.PWM_value=newPWM_value;
        }
        vTaskDelay(pdMS_TO_TICKS(7));
    }
}

// as long as the target pressure is within the calibration range, run a binary search
// WARNING: binary search assumes the values sorted (aka monotonic)
int mapPos(float P_target) {
    // Determine the actual pressure range from the calibration data
    float P_min = sharedData.pmap[sharedData.PWM_c_min];
    float P_max = sharedData.pmap[sharedData.PWM_c_max];
    // if(sharedData.wave_debug) Serial.printf("Pm: %2.2f, PM: %2.2f, PT:%2.2f\n",P_min,P_max,P_target);
    // Clamp P_target to be within the actual pressure range
    if(P_target<P_min || P_target>P_max){
        sharedData.error=3;
        Serial.printf("P_m:%2.2f,P_M:%2.2f exceed calibration range, consider recalibrating\n",P_min,P_max);
    }
    P_target = constrain(P_target, P_min, P_max);
    // Binary search to find closest pressure
    int left = sharedData.PWM_c_min;
    int right = sharedData.PWM_c_max;
    int bestPos = left;
    float minDiff = P_max-P_min;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        // Skip uncalibrated positions
        if (sharedData.pmap[mid] == -1.0) {
        // Try to find the nearest calibrated position
        int searchLeft = mid - 1;
        int searchRight = mid + 1;
        bool found = false;
        
        // Find nearest calibrated position
        while (!found && (searchLeft >= sharedData.PWM_c_min || searchRight <= sharedData.PWM_c_max)) {
            if (searchLeft >= sharedData.PWM_c_min && sharedData.pmap[searchLeft] != -1.0) {
                mid = searchLeft;
                found = true;
            } else if (searchRight <= sharedData.PWM_c_max && sharedData.pmap[searchRight] != -1.0) {
                mid = searchRight;
                found = true;
            } else {
                searchLeft--;
                searchRight++;
            }
        }
        
        // If no calibrated position found, break
        if (!found) break;
        }
        
        float currentP = sharedData.pmap[mid];
        float diff = abs(currentP - P_target);
        
        // Update best position if we found a closer match
        if (diff < minDiff) {
            minDiff = diff;
            bestPos = mid;
        }
        
        // Perfect match found
        if (diff == 0) return mid;
        // Decide which half to search next
        if (currentP < P_target) left = mid + 1;
        else right = mid - 1;
    }
    // if(sharedData.wave_debug) Serial.printf("T_P:%f\tP_min:%f\tP_max%f\tT_PWM:%f\n",P_target,sharedData.P_min,sharedData.P_max,bestPos);
    return bestPos;
}