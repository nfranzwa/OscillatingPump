#include <PA12.h>
#define ID_NUM 0
#define DUTY_CYCLE 35  // Fixed 35% duty cycle
#define MIN_POSITION 10
#define MAX_POSITION 2010

PA12 myServo(&Serial1, 2, 1);
//        (&Serial, enable_pin, Tx Level)

int currentPosition;
int targetPosition = MIN_POSITION;  // Start with minimum position
unsigned long lastMoveTime = 0;
unsigned long restStartTime = 0;
bool isResting = false;
bool isMoving = false;

// Calculate rest time based on active time for 35% duty cycle
unsigned long calculateRestTime(unsigned long activeTime) {
  return (activeTime * (100 - DUTY_CYCLE)) / DUTY_CYCLE;
}

void setup() {
  Serial.begin(9600);    
  myServo.begin(32);  
  while (!Serial);
  
  // Set initial speed (you can adjust this value)
  myServo.movingSpeed(ID_NUM, 50);
  
  Serial.println("=== Oscillating Linear Actuator ===");
  Serial.println("Oscillating between positions 10 and 2010");
  Serial.println("Operating at 35% duty cycle");
  Serial.println("Press any key to stop the oscillation");
}

void loop() {
  // Check for stop command
  if (Serial.available()) {
    Serial.println("\nStopping oscillation!");
    while(1); // Infinite loop to stop the program
  }
  
  // Only start new movement if we're not resting and not currently moving
  if (!isResting && !isMoving) {
    // Start movement to target position
    unsigned long moveStartTime = millis();
    myServo.goalPosition(ID_NUM, targetPosition);
    isMoving = true;
    
    Serial.print("Moving to position: ");
    Serial.println(targetPosition);
    
    // Monitor position while moving
    while (myServo.Moving(ID_NUM)) {
      currentPosition = myServo.presentPosition(ID_NUM);
      Serial.print("  - Current Position: ");
      Serial.println(currentPosition);
      delay(50);
    }
    
    // Movement complete
    isMoving = false;
    
    // Calculate active time and required rest time
    unsigned long activeTime = millis() - moveStartTime;
    unsigned long requiredRestTime = calculateRestTime(activeTime);
    
    Serial.print("  - Final Position: ");
    Serial.println(myServo.presentPosition(ID_NUM));
    
    // Enter rest period
    isResting = true;
    restStartTime = millis();
    Serial.print("Resting for ");
    Serial.print(requiredRestTime);
    Serial.println("ms to maintain duty cycle");
    
    // Toggle target position for next movement
    targetPosition = (targetPosition == MIN_POSITION) ? MAX_POSITION : MIN_POSITION;
  }
  
  // Check if rest period is over
  if (isResting) {
    unsigned long currentTime = millis();
    unsigned long restTime = currentTime - restStartTime;
    unsigned long requiredRestTime = calculateRestTime(lastMoveTime);
    
    if (restTime >= requiredRestTime) {
      isResting = false;
      Serial.println("Rest complete - Starting next movement");
    }
  }
}