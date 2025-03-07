#ifndef SENSOR_H
#define SENSOR_H
#include <Arduino.h>
#include <filter_lib.h>
class PSensor {
    private:
        const int SENSOR_PIN;
        const float V_SUPPLY;
        const float V_MIN;
        const float V_MAX;
        const float P_MAX;
        const float P_MIN;
        const int ADC_RESOLUTION;
        String SENSOR_TYPE;
        // const int I2C_ADDRESS=0x27;
        const int I2C_ADDRESS=0x28;
        // cmd to request measurement
        const byte CMD[3] = {0xAA,0x00,0x00};
        const double outputMax = 15099494;  // 90% of 2^24 counts
        const double outputMin = 1677722;   // 10% of 2^24 counts
        const double pressureMax = 6.0;     // 6 kPa maximum pressure
        const double pressureMin = 0.0;     // 0 kPa minimum pressure
        double P_atm = 0.0;                 // init pressure reading/calibration
        int praw=0;

        //filter parameters
        float past_estimate     = 0.0;
        float past_error        = 1.0;
        const float NOISE_PROC  = 0.1; //process noise
        const float NOISE_MEAS  = 0.2; //measurement noise
    public:
        PSensor(int pin,String sensor_type, float v_s, float v_min, float v_max, float p_max, float p_min=0.0, int res=4095) : 
                SENSOR_PIN(pin),SENSOR_TYPE(sensor_type), V_SUPPLY(v_s),V_MIN(v_min),V_MAX(v_max), P_MAX(p_max), P_MIN(p_min), ADC_RESOLUTION(res) {}
        void begin();
        float readPressure();
        // float readPressureABP();
        float maptopsi(int value);
        float filter(float measurement);
        lowpass_filter LPF=lowpass_filter(2.0);
        // void sensor_debug(bool debug=false);
};
#endif