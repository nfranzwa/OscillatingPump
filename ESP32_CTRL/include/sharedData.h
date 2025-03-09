#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <Arduino.h>

// Shared data structure
struct SharedData {
    float P_current;        // Current pressure reading
    int PWM_value;          // Current PWM value of actuation
    int ASDR[4];            // ASDR values (Attack, Sustain, Decay, Rest)
    int cyc_period;         // cycle period in mss
    int PWM_max;            // Maximum PWM value of actuation
    int PWM_min;            // Minimum PWM value of actuation
    float PID[3];           // PID coefficients
    String mode_current;    // Current mode
    String param_current;   // Current parameter being edited
    int value_current;      // Current value of the parameter
    String err_msg="";
    float pmap[4095]={-1.0};       // pressure mapped onto each position
    float P_minH2O;
    float P_maxH2O;
    /*
    0: not calibrated
    1: calibrating
    2: calibrated, automatic
    3: manual
    4: error
    */
    int calibration_state;
    int PWM_manual;
    int PWM_c_min=0;
    int PWM_c_max=4095;
    // int PWM_current;
    //TODO: correction code;
    /*
    wave generation must interface with following vars:
    - last_remembered_min_pos
    - min-max offset obtained from calibration values (aka pmap)
    Generate wave on
    - feedback_min_pos_remembered 
    - feedback_min_pos_remembered +min_max_offset_from_calibration
    In the loop:
    @ the beginning of each Attack:
    - remember the highest position below min pressure;
    @ the beginning of each Decay (optional):
    - remember lowest position above max pressure;

    */
    //updated to be PWM_c_min during calibration=1, stage 2
    // TODO: this value is updated by wavegen.cpp
    int PWM_last_min=0;
    // TODO: update this value from TF_wavegen
    int PWM_offset=1000;
    float P_min=1.0;
    float P_max=6.0;
    //based on sensor specs

    float P_test;
    /*
    0: not calibrated
    1: calibrating
    2: calibrated, automatic
    3: manual
    4: error
    */
    int STATUS_PINS[5] ={1,2,3,4,5};
};

extern SharedData sharedData;  // Declare the global instance of SharedData
#define CLK         26
#define DT          27
#define SW          14
#define P_PIN       12 // pressure sensor output pin
#define P_PIN_NEW   34 // new pressure sensor tested
#define P_PIN_OUT   2  // pin to output pressure reading to
#define PIN_IN      15
// for I2C sensor
#define P_PIN_SCL   35
#define P_PIN_SDA   34
#define I2C_ADDRESS 0x28  // I2C address of the pressure sensor

#define V_SUPPLY    3.3
#define V_MIN       0.1
#define V_MAX       3.08
#define P_MIN       -1.12
#define P_MAX       1.12 // max pressure sensor output
#define ADC_RES     4095

//display
// For I2C sensor
#define SDA         21 // S(erial) DA(ta):  Display
#define SCL         22 // S(erial) CL(ock): Display
// #define EOC         7  // end of conversion pin


#define TXD1        18 // ESP serial communication
#define RXD1        19

// Constants from Agasthya's UI
#define ZERO_SAMPLES 50
#define COUNTS_PER_PSI 735990  // Increased to further scale down PSI values
#define PSI_TO_CMH2O 70.307    // Conversion factor from PSI to cmH2O
#define DRIFT_THRESHOLD 0.1    // PSI threshold to consider as "no pressure"
#define STABLE_TIME 3000       // Time in ms that readings need to be stable for re-zeroing
#define DRIFT_SAMPLES 10       // Number of samples to check for drift

// motor control
constexpr int ID_NUM = 1;
constexpr int MIN_POS= 100;
constexpr int MAX_POS= 4095;
constexpr int MIGHTY_ZAP_RX= 16;
constexpr int MIGHTY_ZAP_TX= 17;
constexpr int MIGHTY_ZAP_EN= 13;
constexpr int LOOP_DELAY = 3;

#endif
