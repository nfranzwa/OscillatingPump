#include "RotaryEncoder.h"
#include <Arduino.h>

void RotaryEncoder::begin() {
    pinMode(CLK, INPUT);
    pinMode(DT, INPUT);
    pinMode(SW, INPUT_PULLUP);
    last_s_CLK = digitalRead(CLK);
}

void RotaryEncoder::setOptions(int* values, String* names, int inc) {
    optValues=values;
    optNames = names;
    optSize = sizeof(values); //simple pointer arithmetic
    increment=inc;
}

void RotaryEncoder::update(bool debug) {
    s_CLK = digitalRead(CLK);
    
    // Avoid double count, detect a pulse/change
    if (s_CLK != last_s_CLK && s_CLK == 1) {
        if (digitalRead(DT) != s_CLK) {
            edit_mode ? optValues[mod(position,optSize)]-=increment : position--;
            s_dir="CCW";
        } else {
            edit_mode ? optValues[mod(position,optSize)]+=increment : position++;
            s_dir = "CW";
        }
        if(debug) Serial.printf("Variable: %-10s\tValue: %.2f\n",
            getCurrentName().c_str(),
            getCurrentValue());
    }
    last_s_CLK = s_CLK; // remember last clk state
    
    // Handle button press
    int btn_state = digitalRead(SW);
    if (btn_state == LOW) {
        if (millis() - lastButtonPress > DEBOUNCE_DELAY) {
            edit_mode = !edit_mode;
            Serial.printf("%s:%s\n", 
                         edit_mode ? "EDIT" : "SAVED",
                         optNames[mod(position, optSize)].c_str());
        }
        lastButtonPress = millis();
    }
    
    //delay(LOOP_DELAY); // Help debounce reading with slight delay
}