#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <Arduino.h>

// Shared data structure
struct SharedData {
    float P_current;        // Current pressure reading
    float P_target;         // Target pressure
/*
  WARNING: Know that increasing PWM values corresponds with minimizing pressure.
  For the sake of convention + retained understanding, we do the flip when
  - sending information to the motor.
  - updating the values of targets
*/
    int PWM_value;          // Current PWM value
    int ASDR[4];            // ASDR values (Attack, Sustain, Decay, Rest)
    int cyc_period;         // cycle period in mss
    int PWM_max=4095;            // Maximum PWM value
    int PWM_min=100;            // Minimum PWM value
    float PID[3];           // PID coefficients
    String mode_current;    // Current mode
    String param_current;   // Current parameter being edited
    int value_current;      // Current value of the parameter
    String err_msg="";
    // if the values are negative 1 then it means the values were not initialized.
    // pressure mapped onto each position
    float pmap[4095]={-1.0};
    /* Calibration States:
        0 - not calibrated
        1 - running calibration
        2 - calibration finished
        3 - running manual 
    */
    int calibration_state = 0;
    float P_min = 0.3;
    float P_max = 5.0;
    //pwm min and max values obtained from calibration
    float PWM_c_min;
    float PWM_c_max;
    int manualPWM;
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
#define P_PIN_SDL 35
#define P_PIN_SDA 34
#define I2C_ADDRESS 0x28  // I2C address of the pressure sensor

#define V_SUPPLY    3.3
#define V_MIN       0.1
#define V_MAX       3.08
#define P_MIN       0.0
#define P_MAX       10.0 // max pressure sensor output
#define ADC_RES     4095

#define SCL         22 // S(erial) CL(ock): Display
#define SDA         31 // S(erial) DA(ta):  Display



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
