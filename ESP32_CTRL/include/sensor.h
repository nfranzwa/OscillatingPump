#ifndef SENSOR_H
#define SENSOR_H
#include <Arduino.h>

class PSensor {
    private:
        const int SENSOR_PIN;
        const float V_SUPPLY;
        const float V_MIN;
        const float V_MAX;
        const float P_MAX;
        const int ADC_RESOLUTION;
        
        //filter parameters
        float past_estimate     = 0.0;
        float past_error        = 1.0;
        const float NOISE_PROC  = 0.1; //process noise
        const float NOISE_MEAS  = 0.2; //measurement noise
    public:
        PSensor(int pin,float v_s, float v_min, float v_max, float p_max,int res) : 
                SENSOR_PIN(pin), V_SUPPLY(v_s),V_MIN(v_min),V_MAX(v_max), P_MAX(p_max),ADC_RESOLUTION(res) {}
        void begin();
        float readPressure();
        float filter(float measurement);
        // void sensor_debug(bool debug=false);
};
#endif