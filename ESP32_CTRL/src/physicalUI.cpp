#include <physicalUI.h>
#include <Arduino.h>
#include "sharedData.h"
#include <wavegen.h>
void PhysicalUI::begin() {
    pinMode(CLK, INPUT);
    pinMode(DT, INPUT);
    pinMode(SW, INPUT_PULLUP);
    
    /* for(int i=0;i<4;i++){
        pinMode(sharedData.STATUS_PINS[i],OUTPUT);
    } */
    last_s_CLK = digitalRead(CLK);
}

void PhysicalUI::setOptions(int* values, String* names, int inc) {
    optValues=values;
    optNames = names;
    optSize = sizeof(values); //simple pointer arithmetic
    increment=inc;
}

void PhysicalUI::handleButton(){
    // int btn_state=digitalRead(SW)
    // if button state changes from previous stored
    if(btn_state!=digitalRead(SW)){
        //button is pushed
        if(btn_state==LOW) lastButtonPress=millis(); //start tracking time of press
        //button is released
        else {
            //revert mode to previous
            if(press_length>DEBOUNCE_DELAY && press_length<MAX_BTN_CLICK){
                view_mode = (view_mode + 1) % NUM_VIEW_MODES;
                // Serial.printf("CYCLE\t", sharedData.mode_current.c_str());
            }
            sharedData.mode_current=getViewMode();
            // Serial.printf("BTN released, %s\n",sharedData.mode_current);
        }
    } else{ // button is same as before
        //if held down
        if(btn_state==LOW){
            press_length=millis()-lastButtonPress;
            //evaluate how long its been down
            if((int) press_length >MAX_BTN_CLICK){
                //it is being held
                sharedData.mode_current="EDIT";
            }
        }
        //resting up
    }
    btn_state=digitalRead(SW);
}

void PhysicalUI::handleEncoder(){
    s_CLK = digitalRead(CLK);
    // detect a pulse/change
    if (s_CLK != last_s_CLK && s_CLK == 1) {
        //Edit mode: modify current param values
        if (sharedData.mode_current=="EDIT"){
            if (digitalRead(DT) != s_CLK) {
                sharedData.value_current-=increment;
            } else {
                sharedData.value_current+=increment;
                // (sharedData.mode_current=="EDIT") ? optValues[mod(position,optSize)]+=increment : position++;
            }
            // only update the value if it is within safe threshold
            // Motor will throw a fit if it is running half the time;
            // basically 
            if(sharedData.value_current>=100) {
                // if(sharedData.ASDR[0]+sharedData.ASDR[2]>sharedData.ASDR[1]+sharedData.ASDR[3]){
                //     sharedData.err_msg="SAFETY: ↓A+D or ↑S+R";
                // }
                // else{
                //     sharedData.err_msg="";
                // }
                optValues[mod(position,optSize)]=sharedData.value_current;
                sharedData.ASDR[mod(position,optSize)]= sharedData.value_current;
            }
        }
        else{ //other view modes
            // cycle through parameters
            (digitalRead(DT)!=s_CLK) ? position-- : position++;
            
        }
        sharedData.param_current = optNames[mod(position, optSize)];
        sharedData.value_current = optValues[mod(position, optSize)];
    }
    last_s_CLK = s_CLK; // remember last clk state
}

void PhysicalUI::update(bool debug) {
    handleButton();
    handleEncoder();
    if(debug) Serial.printf("Variable: %-10s\tValue: %.2f\n",
        getCurrentName().c_str(),
        getCurrentValue());
    // delay(LOOP_DELAY); // Help debounce reading with slight delay
}

//currently not used
void PhysicalUI::updateLCD(float pressure, float target,bool debug){
    if (lcd==nullptr) {
        if (debug) Serial.println("LCD not instanced. (null ptr)\n");
        return;
    }
    if (millis()-lastLCDUpdate<LCD_UPDATE_INTERVAL){
        return;
    }
    lcd->setCursor(0,0);
    lcd->printf("MODE:%-4s",
        getViewMode().c_str());
    lcd->setCursor(0,1);
    lcd->printf("%-7s:%5d ms", getCurrentName().c_str(), getCurrentValue());
    lcd->setCursor(0,2);
    lcd->printf("T1:%-4.0f P:%-3.2f kPa",target,pressure);
    lastLCDUpdate=millis();
    //TODO: Error message/instructions

    /*
    if(getViewMode()=="VIEW"){}
    else if (getViewMode()=="EDIT"){}
    else if (getViewMode()="TEMP"){}
    else{}
    */
}
/*
update status LED based on calibration state variable
    0: not calibrated
    1: calibrating
    2: calibrated, automatic
    3: manual
    4: error
*/
void TF_status_LED(void* pvParams){
    Serial.println("start status LED task");
    for(;;){
        for(int i=0;i<4;i++){
            if(sharedData.calibration_state==i) digitalWrite(sharedData.STATUS_PINS[i],HIGH);
            else digitalWrite(sharedData.STATUS_PINS[i],LOW);
        }
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

void TF_ui(void* pvParams){
    PhysicalUI* ui= (PhysicalUI*) pvParams;
    for(;;){
        ui->update();
        sharedData.mode_current=ui->getViewMode();
        sharedData.param_current=ui->getCurrentName();
        sharedData.value_current=ui->getCurrentValue();
        //Serial.printf("%s: %s\t%d\n",sharedData.mode_current,sharedData.param_current,sharedData.value_current);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void TF_lcd(void* pvParams){
    LiquidCrystal_I2C* lcd=(LiquidCrystal_I2C*) pvParams;
    for(;;){

        lcd->setCursor(0, 0);
        lcd->printf("MODE:%-4s", sharedData.mode_current.c_str());
        lcd->setCursor(0, 1);
        lcd->printf("%-7s:%5d ms", sharedData.param_current.c_str(), sharedData.value_current);
        lcd->setCursor(0, 2);
        lcd->printf("T:%-5d P:%+2.2f psi", sharedData.PWM_value, sharedData.P_current);
        lcd->setCursor(0, 3);
        lcd->printf("Pm:%+2.2f PM:%+2.2f", (float) sharedData.P_min, sharedData.P_max);
        
        // lcd->setCursor(0,3);
        // lcd->scrollDisplayRight();
        // lcd->printf("%s",(sharedData.err_msg=="")? "                    ":sharedData.err_msg);
        // Serial.printf("T1:%-4.0f P:%-2.2f kPa\n", (float) sharedData.PWM_value, sharedData.P_current);
        // updateLCD(sharedData.P_current,(float) sharedData.PWM_value);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}