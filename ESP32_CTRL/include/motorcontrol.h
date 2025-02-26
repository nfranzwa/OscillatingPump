#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H
#include <Arduino.h>
#include <MightyZap.h>

class MotorControl {
    private:
        int maxPosition=2000; //default max position
        float frequency=0.75; //default frequency in Hz
        unsigned long lastSensorRead;
        unsigned long cycleStartTime;
        const float DUTY_CYCLE = 0.4;
    
        const int ID_NUM    = 1; //actuator number (for multiple actuators)
        int MIN_POS         =100; //actuator position min and max
        int MAX_POS         =4095; //actually will be defined by user
        const int MIGHTY_ZAP_RX; //pins
        const int MIGHTY_ZAP_TX; //pins
        const int MIGHTY_ZAP_EN; //pins
    public:
        MotorControl(int RX, int TX, int EN):
        MIGHTY_ZAP_RX(RX), MIGHTY_ZAP_TX(TX),MIGHTY_ZAP_EN(EN) {};
        MightyZap* m_zap;

        void setMotor(MightyZap* mz){ m_zap=mz;}
        void begin();
        int calculateSpeed(int distance, float freq);
        void handleSerialCommands();
        void handleMovement();
        void updateParameters();
        void readAndPrintSensor();
        void TH_motor(void* pvParams);
        void mapPressure();
        void calibrate(int state);
    };

#endif