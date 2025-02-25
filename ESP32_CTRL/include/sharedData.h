#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <Arduino.h>

// Shared data structure
struct SharedData {
    float P_current;        // Current pressure reading
    float P_target;         // Target pressure
    int PWM_value;          // Current PWM value
    int ASDR[4];            // ASDR values (Attack, Sustain, Decay, Rest)
    int cyc_period;         // cycle period in mss
    int PWM_max;            // Maximum PWM value
    int PWM_min;            // Minimum PWM value
    float PID[3];           // PID coefficients
    String mode_current;    // Current mode
    String param_current;   // Current parameter being edited
    int value_current;      // Current value of the parameter
    String err_msg="";
    float pmap[4095];       // pressure mapped onto each position
};

extern SharedData sharedData;  // Declare the global instance of SharedData

#endif