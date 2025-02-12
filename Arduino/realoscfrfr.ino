#include <MightyZap.h>
#define ID_NUM 0
#define MIN_POS 500

// Initialize MightyZap with Serial1, pin 2, and timeout of 1
MightyZap m_zap(&Serial1, 2, 1);

int maxPosition = 2500;  // Default max position
float frequency = 1.0;   // Default frequency in Hz

// Calculate required speed for 50% duty cycle
int calculateSpeed(int distance, float freq) {
  // Speed in MightyZap units (0-1023) needed to cover the distance in half the period
  // Period = 1/frequency, half period = 1/(2*frequency)
  // MightyZap speed units are approximately mm/sec * 10
  float timePerCycle = 1.0 / (2.0 * freq);  // Time for one direction (half cycle)
  float requiredSpeed = (float)distance / timePerCycle / 10.0;  // Convert to MightyZap units
  return (int)constrain(requiredSpeed, 1, 1023);
}

void updateParameters() {
  int distance = maxPosition - MIN_POS;
  int speed = calculateSpeed(distance, frequency);
  m_zap.GoalSpeed(ID_NUM, speed);
}

void setup() {
  Serial.begin(9600);    
  m_zap.begin(32);  
  updateParameters();
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    
    if (command.startsWith("P:")) {
      maxPosition = command.substring(2).toInt();
      updateParameters();
    }
    else if (command.startsWith("F:")) {
      frequency = command.substring(2).toFloat();
      updateParameters();
    }
  }
  
  // Oscillate between min and max positions
  m_zap.GoalPosition(ID_NUM, MIN_POS);
  while(m_zap.Moving(ID_NUM)) {
    delay(10);
  }
  
  m_zap.GoalPosition(ID_NUM, maxPosition);
  while(m_zap.Moving(ID_NUM)) {
    delay(10);
  }
}