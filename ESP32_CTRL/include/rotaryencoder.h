#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H
#include <Arduino.h>

class RotaryEncoder {
    private:
        const int CLK;
        const int DT;
        const int SW;
        
        int s_CLK; // current state of click
        int last_s_CLK; //last state of click
        int position; //encoder position
        bool edit_mode;
        String s_dir; //direction of rotation
        unsigned long lastButtonPress=0.0; // time of last button press
        // Menu options
        int* optValues;
        String* optNames;
        int optSize;
        int currentOpt;
        int increment;
        static constexpr unsigned int DEBOUNCE_DELAY=50; //ms
        
        // helper funct for scrolling in negative values
        int mod(int a, int b) const {
        return ((a % b) + b) % b;
    }
    public:
        RotaryEncoder(int clk, int dt, int sw) : 
            CLK(clk), DT(dt), SW(sw),
            s_CLK(0),last_s_CLK(0),position(0),
            edit_mode(false),s_dir(""),lastButtonPress(0.0),
            optValues(nullptr),optNames(nullptr),
            optSize(0),currentOpt(0),increment(0.0) {}
        void begin();
        //IF we want to use s instead of ms, change to double
        void setOptions(int* values, String* names, int inc);
        void update(bool debug=false);
        int getPosition() { return position; }
        int getCurrentValue() {return optValues[mod(position,optSize)];}
        String getCurrentName() {return optNames[mod(position,optSize)];}
        bool isEditMode() {return edit_mode;}
};
#endif