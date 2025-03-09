#include <sensor.h>
#include <Arduino.h>
#include "sharedData.h"
#include <Wire.h>
#include <filter_lib.h>
#include <motorcontrol.h>

void PSensor::begin(){
    analogReadResolution(12); //default 12 bits (0~4095)
    analogSetAttenuation(ADC_11db);
    // if(SENSOR_TYPE=="I2C") pinMode(EOC,INPUT);
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
    else if(SENSOR_TYPE=="I2C"){
        /*
            TODO: UPDATE WHICH SENSOR THIS IS
            THIS IS PSI READINGS
        */
        // send request for measurements
        Wire.beginTransmission(I2C_ADDRESS);
        int stat = Wire.write(CMD, 3);
        if(Wire.endTransmission()==0){
            // Serial.println("Pressure sensor found at 0x28\n");
        }
        else Serial.println("Sensor not found\n");
        Wire.requestFrom(I2C_ADDRESS, 7); //7 byte total
        vTaskDelay(5);
        if(Wire.available()>=7){
            // Serial.println("Sensor found?");
            int i=0;
            Wire.requestFrom(I2C_ADDRESS, 7);
            for (i = 0; i < 7; i++)
            {
                data[i] = Wire.read();
            }
            press_counts = data[3] + data[2] * 256 + data[1] * 65536; // calculate digital pressure counts
            pressure = ((press_counts - outputMin) * (pmax - pmin)) / (outputMax - outputMin) + pmin;
            return pressure; // PSI VALUE
        }
        else{
            Serial.println("No msg read\n");
            return P_MAX;
        }
    }
    else if (SENSOR_TYPE=="I2C2B"){
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
    else if(SENSOR_TYPE=="DUMMY"){
        return sharedData.P_test;
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
    bool useLPF=true;

    if(useLPF){
        return LPF.filter(measurement);
    }
    else{
        /*
        Digital filter to apply to the sensor measurements.
        Currently trying to implement a Kalman filter.
        */
        
        float prediction = past_estimate;
        float prediction_err= past_error+NOISE_PROC; // prediction error
        
        float gain_kalman= prediction_err/(prediction_err+NOISE_MEAS);
        float estimate= prediction+gain_kalman*(measurement-prediction);
        float estimate_err = (1-gain_kalman)*prediction_err;
        
        past_estimate=estimate;
        past_error=estimate_err;
        return estimate;
    }
}

void TF_sensor(void *pvParams){
    //generic data is sent (void*) so we type cast
    PSensor* sensor =(PSensor*) pvParams;
    for(;;){
        // WARNING: remember that I2C sensor readings are in PSI

        sharedData.P_current=sensor->filter(sensor->readPressure());
        // Serial.printf("Sensor data:%-5f\n",sharedData.P_current);
        vTaskDelay(pdMS_TO_TICKS(15)); //66.7Hz
    }
}

void mapPressure(void* pvParams) {
    MotorControl* motor= (MotorControl*) pvParams;
    for(;;){
      sharedData.pmap[motor->m_zap->presentPosition(ID_NUM)]=sharedData.P_current;
      vTaskDelay(pdMS_TO_TICKS(33));//30Hz
    }
}

void cleanArray(std::array<float,4096>& pmap, int startIdx, int endIdx) {
    // Skip processing if the range is invalid
    if (startIdx >= endIdx) return;
    
    // First pass: Find the first valid value
    int firstValid = -1;
    for (int i = startIdx; i <= endIdx; i++) {
        if (pmap[i] != -1.0f) {
            firstValid = i;
            break;
        }
    }
    
    // Return if no valid points were found
    if (firstValid == -1) return;
    
    // Second pass: Ensure monotonicity (never decreasing)
    float currentMax = pmap[firstValid];
    
    // Set all invalid entries before firstValid to the first valid value
    for (int i = startIdx; i < firstValid; i++) {
        pmap[i] = currentMax;
    }
    
    // Process the rest of the array
    for (int i = firstValid + 1; i <= endIdx; i++) {
        if (pmap[i] == -1.0f) {
            // Replace invalid values with the current maximum
            pmap[i] = currentMax;
        } else {
            // If valid but less than current maximum, update to current maximum
            if (pmap[i] < currentMax) {
                pmap[i] = currentMax;
            } else {
                // Update current maximum if we found a larger value
                currentMax = pmap[i];
            }
        }
    }
}

/*
Replicate test sensor data according to message sent to serial:
Feed it pressure value and amount to increment each iteration
in the following format: "TP: ##.###, I:#.###"
it should update the values based on the last message received,
and write to sharedData.P_test
*/
void TF_ptest(void* pvParams){
    float P_val=2.0;//test pressure value
    float P_inc=-0.0;//test pressure increment
    String serialInput;
    sharedData.P_test=P_val;
    bool printVals=false;
    for (;;){
        //scan serial port for message like example
        if(Serial.available()){
            serialInput=Serial.readStringUntil('\n');
            //if there's a new message that matches, rewrite the variables
            if (sscanf(serialInput.c_str(), "TP:%f, I:%f", &P_val, &P_inc) == 2) {
                // Successfully parsed the message
                Serial.printf("Updated: TP = %-4.4f,I = %-4.4f\n",P_val,P_inc);
                sharedData.P_test=P_val;
            }
            if (serialInput.startsWith("Toggle")){
                printVals=!printVals;
            }
        }
        //otherwise continue changing the sharedData.P_test as normal
        sharedData.P_test+=P_inc;
        if(printVals) Serial.printf("Sensor Data:%-5f,Generated:%-5f\n",
            sharedData.P_current,sharedData.P_test);
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}