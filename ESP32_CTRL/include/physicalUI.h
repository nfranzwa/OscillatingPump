#ifndef PHYSICAL_UI_H
#define PHYSICAL_UI_H
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
class PhysicalUI {
    private:
        const int CLK;
        const int DT;
        const int SW;
        LiquidCrystal_I2C* lcd;
        int s_CLK; // current state of click
        int last_s_CLK; //last state of click
        int position; //encoder position
        unsigned long lastButtonPress=0.0; // time of last button press
        unsigned long lastLCDUpdate=0.0;
        // Menu options
        int* optValues;
        String* optNames;
        int optSize;
        int currentOpt;
        int increment;

        int view_mode;
        static const int NUM_VIEW_MODES=3;
        const String view_modes[NUM_VIEW_MODES]={"VIEW","EDIT","TEMP"};
        // static constexpr unsigned int LOOP_DELAY=3;
        static constexpr unsigned int DEBOUNCE_DELAY=3; //ms
        static constexpr unsigned int CLICK_DELAY=50; //ms
        static constexpr unsigned int LCD_UPDATE_INTERVAL=1000; //ms
        
        // helper funct for scrolling in negative values
        int mod(int a, int b) const { return ((a % b) + b) % b;}
        //task handles
        TaskHandle_t encoder_TH;
        TaskHandle_t lcd_TH;

        static void encoderTask(void* parameter);
        static void lcdTask(void* parameter);
    public:
        PhysicalUI(int clk, int dt, int sw) : 
            CLK(clk), DT(dt), SW(sw),
            s_CLK(0),last_s_CLK(0),position(0),
            view_mode(0),lastButtonPress(0.0),
            optValues(nullptr),optNames(nullptr),
            optSize(0),currentOpt(0),increment(0.0),
            encoder_TH(nullptr), lcd_TH(nullptr) {}
        void begin();
        void handleEncoder();
        void handleButton();
        //share the LCD instance created in main.cpp
        void setLCD(LiquidCrystal_I2C* lcd_display){ lcd=lcd_display;}
        //IF we want to use s instead of ms, change to double
        void setOptions(int* values, String* names, int inc);
        void update(bool debug=false);
        void updateLCD(float pressure=0.0,float target=0.0,bool debug=false);
        int getPosition() { return position; }
        int getCurrentValue() {return optValues[mod(position,optSize)];}
        String getCurrentName() {return optNames[mod(position,optSize)];}
        String getViewMode() {return view_modes[mod(view_mode,3)];}
};
#endif
