#include <physicalUI.h>
#include <Arduino.h>

void PhysicalUI::begin() {
    pinMode(CLK, INPUT);
    pinMode(DT, INPUT);
    pinMode(SW, INPUT_PULLUP);
    last_s_CLK = digitalRead(CLK);

    xTaskCreatePinnedToCore(
        lcdTask,   //task funct
        "LCD_task",    //task name
        4096,           //stack size
        this,           //task parameter
        1,              //task priority
        &lcd_TH,         //handle for tracking task
        0               //pin task to core #
    );
    // delay(100);
    xTaskCreatePinnedToCore(
        encoderTask,   //task funct
        "encoder_task",    //task name
        4096,           //stack size
        this,           //task parameter
        1,              //task priority
        &encoder_TH,         //handle for tracking task
        1               //pin task to core #
    );
    // delay(100);
}

void PhysicalUI::setOptions(int* values, String* names, int inc) {
    optValues=values;
    optNames = names;
    optSize = sizeof(values); //simple pointer arithmetic
    increment=inc;
}

void PhysicalUI::handleButton(){
    int btn_state = digitalRead(SW);
    if (btn_state == LOW) {
        if (millis() - lastButtonPress > CLICK_DELAY) {
            view_mode= (view_mode++) % NUM_VIEW_MODES;
            Serial.printf("%s:%s\n", 
                         (getViewMode()=="EDIT") ? "EDIT" : "SAVED",
                         optNames[mod(position, optSize)].c_str());
        }
        lastButtonPress = millis();
    }
}

void PhysicalUI::handleEncoder(){
    s_CLK = digitalRead(CLK);
    // detect a pulse/change
    if (s_CLK != last_s_CLK && s_CLK == 1) {
        if (digitalRead(DT) != s_CLK) {
            (getViewMode()=="EDIT") ? optValues[mod(position,optSize)]-=increment : position--;
        } else {
            (getViewMode()=="EDIT") ? optValues[mod(position,optSize)]+=increment : position++;
        }
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
    lcd->printf("T1:%-4.0f P:%-2.2f kPa",target,pressure);
    lastLCDUpdate=millis();
    //TODO: Error message/instructions

    /*
    if(getViewMode()=="VIEW"){}
    else if (getViewMode()=="EDIT"){}
    else if (getViewMode()="TEMP"){}
    else{}
    */
}


void PhysicalUI::encoderTask(void* parameter) {
    /*  These are static task handlers, owned by the class
        The principle is to run this task on one core, 
        and have this code constantly running
    */
    PhysicalUI* ui = (PhysicalUI*) parameter;
    
    for(;;) {
        ui->handleEncoder();
        ui->handleButton();
        delay(DEBOUNCE_DELAY);
    }
}

void PhysicalUI::lcdTask(void* parameter) {
    PhysicalUI* ui = (PhysicalUI*) parameter;
    for (;;) {
        ui->updateLCD(0.0, 0.0, false); // Update with actual pressure and target values
        delay(LCD_UPDATE_INTERVAL);
    }
}