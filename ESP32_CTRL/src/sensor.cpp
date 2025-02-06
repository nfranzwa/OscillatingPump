#include <sensor.h>
#include <Arduino.h>

void PSensor::begin(){
    analogReadResolution(12); //default 12 bits (0~4095)
    analogSetAttenuation(ADC_11db);
}

float PSensor::readPressure(){
    /* from datasheet of MP3V5010:
    V_OUT=V_S*(.09*P+0.08) \pm V_S*(0.09*P_error * temp_factor)
    ignoring the noise section:
    → P = (V_OUT/V_S-.08)/.09
    */
    float adc_value=analogRead(SENSOR_PIN);
    float voltage= (adc_value/ADC_RESOLUTION)*V_MAX;
    float pressure= (voltage/(V_SUPPLY-.08))/.09;
    return pressure;
    //CONSIDER: adding a constrain() to value
}

float PSensor::filter(float measurement){
    /*
    Digital filter to apply to the sensor measurements.
    Currently trying to implement a Kalman filter.
    */

    //TODO: Validate correctness
    float prediction = past_estimate;
    float prediction_err= past_error+NOISE_PROC; // prediction error

    float gain_kalman= prediction_err/(prediction_err+NOISE_MEAS);
    float estimate= prediction+gain_kalman*(measurement-prediction);
    float estimate_err = (1-gain_kalman)*prediction_err;

    past_estimate=estimate;
    past_error=estimate_err;
    return estimate;
}
/*
void PSensor::sensor_debug(bool debug){
    if(debug){
        float p_raw=readPressure();
        Serial.printf("P_raw: %.3f kPa\tP_filter: %.3f\n",p_raw,filter(p_raw));
    }
}
*/