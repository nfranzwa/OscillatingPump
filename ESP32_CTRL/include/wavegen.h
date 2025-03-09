#ifndef WAVE_GENERATOR_H
#define WAVE_GENERATOR_H

class WaveGenerator {
private:
    const int PWM_CHANNEL;
    const int PWM_FREQ;
    const int PWM_RESOLUTION;
    
    // waveform parameters (ms)
    int* ASDR; // rise, sustain, decay, rest
    // int* ASDR_new; // redundant?
    int cyclePeriod;
    int cycleFreq;
    unsigned long lastCycleStart=0;
    int PWM_min;
    int PWM_max;
    
    int PWM_value;
public:
    WaveGenerator(int pwm_min, int pwm_max, int channel = 0, int freq = 5000, int res = 12) :
        PWM_min(pwm_min),PWM_max(pwm_max),  PWM_CHANNEL(channel), PWM_FREQ(freq), PWM_RESOLUTION(res) 
        {
            ASDR = new int[4]{250,250,250,250};
        }
    float getFrequency(){return 1000.0/cyclePeriod;} //frequency in Hz
    int getPeriod(){return ASDR[0]+ASDR[1]+ASDR[2]+ASDR[3];} //period in ms

    void updateParams(int* new_asdr,int pwm_min, int pwm_max);//update the ASDR and PWM range
    int generatePWM();
    void update(int* ADSR,int min_PWM,int max_PWM,bool debug=false);
};
int mapPos(float P_target);
#endif