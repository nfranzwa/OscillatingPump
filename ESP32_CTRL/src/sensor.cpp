#include <sensor.h>
#include <Arduino.h>
#include "sharedData.h"
#include <Wire.h>

void PSensor::begin(){
    analogReadResolution(12); //default 12 bits (0~4095)
    analogSetAttenuation(ADC_11db);
}

float PSensor::readPressure(){
    float adc_value=analogRead(SENSOR_PIN);
    float voltage= (adc_value/ADC_RESOLUTION)*V_MAX;
    if (SENSOR_TYPE=="NXP"){
        /* from datasheet of MP3V5010 sensor sold by NXP:
        https://www.nxp.com/docs/en/data-sheet/MP3V5010.pdf
        V_OUT=V_S*(.09*P+0.08) \pm V_S*(0.09*P_error * temp_factor)
        ignoring the noise section:
        → P = (V_OUT/V_S-.08)/.09
        The constant value would give a reading of .2 V at P=0 kPa
        
        HOWEVER, following code is run with the transfer function updated as:
        V_OUT=V_S*(.09*P+0.05) → P = (V_OUT/V_S-.05)/.09
        */
       return (voltage/V_SUPPLY -.05)/.09;
    }
    else if (SENSOR_TYPE=="ABP"){
        /* From datasheet of ABP Analog pressure sensor:
        https://www.mouser.com/datasheet/2/187/HWSC_S_A0013047928_1-3073376.pdf
        Using the analog version, 
        V_out= .8*V_SUPPLY/(P_MAX-P_MIN) * (P-P_MIN) + .10*V_SUPPLY
        
        Set P_MAX=10 kPa, P_MIN=0 kPa
        */
       return (voltage-0.1*V_SUPPLY)*(P_MAX-P_MIN)/(.8*V_SUPPLY)+P_MIN;
    }
    else if (SENSOR_TYPE=="I2C"){
        int praw=0;
        Wire.requestFrom(I2C_ADDRESS, 2);
        if (Wire.available() >= 2) {
            uint8_t highByte = Wire.read();
            uint8_t lowByte = Wire.read();
            praw= (highByte << 8) | lowByte;
        } else {
            Serial.println("No data received.");
        }
        return maptopsi(praw)*6.89746;
        
    }
    else{
        return P_MAX;
    }
    //CONSIDER: adding a constrain() to value
}

float PSensor::maptopsi(int value){
    if (value < -16383) value = -16383;
    if (value > 16383)  value = 16383;
    // Perform the mapping:
    // We need to scale from a 32766 range (-16383 to 16383) to a 200 range (-100 to 100)
    float result = (value / 16383.0) * 100.0;
    result = result * 0.0145;
    return result;
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

void TF_sensor(void *pvParams){
    //generic data is sent (void*) so we type cast
    PSensor* sensor =(PSensor*) pvParams;
    for(;;){
        sharedData.P_current=sensor->filter(sensor->readPressure());
        vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz
    }
}