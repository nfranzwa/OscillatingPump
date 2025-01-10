#include <PA12.h>
#define ID_NUM 0
#define DUTY_CYCLE 35  // Fixed 35% duty cycle

PA12 myServo(&Serial1, 2, 1);
//        (&Serial, enable_pin, Tx Level)

int Position;
int cPosition;
int Speed;
unsigned long lastMoveTime = 0;
unsigned long restStartTime = 0;
bool isResting = false;

// Calculate rest time based on active time for 35% duty cycle
unsigned long calculateRestTime(unsigned long activeTime) {
  return (activeTime * (100 - DUTY_CYCLE)) / DUTY_CYCLE;
}

void setup() {
  Serial.begin(9600);    
  myServo.begin(32);  
  while (!Serial);
  
  // Set initial speed
  myServo.movingSpeed(ID_NUM, 50);
  
  // Print initial menu
  printMenu();
}

void loop() {  
  if (Serial.available()) {
    char input = Serial.read();
    
    // Only accept commands if we're not in forced rest period
    if (!isResting) {
      switch (input) {
        case 'p':
        case 'P':
          // Position control
          Serial.print("Enter new position [0~4095]: ");
          while (!Serial.available());
          Position = Serial.parseInt();
          Serial.println(Position);
          
          unsigned long moveStartTime = millis();
          myServo.goalPosition(ID_NUM, Position);
          delay(50);
          
          // Monitor position while moving
          while (myServo.Moving(ID_NUM)) {
            cPosition = myServo.presentPosition(ID_NUM);
            Serial.print("  - Current Position: ");
            Serial.println(cPosition);
            delay(50);
          }
          
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
          break;
          
        case 's':
        case 'S':
          // Speed control
          Serial.print("Enter new speed [0~1023]: ");
          while (!Serial.available());
          Speed = Serial.parseInt();
          Serial.print("Setting speed to: ");
          myServo.movingSpeed(ID_NUM, Speed);
          Serial.println(myServo.movingSpeed(ID_NUM));
          break;
          
        case 'm':
        case 'M':
          printMenu();
          break;
          
        default:
          Serial.println("Invalid command! Press 'M' for menu.");
          break;
      }
      
      // Clear any remaining characters in the serial buffer
      while (Serial.available()) {
        Serial.read();
      }
    }
  }
  
  // Check if rest period is over
  if (isResting) {
    unsigned long currentTime = millis();
    unsigned long restTime = currentTime - restStartTime;
    unsigned long requiredRestTime = calculateRestTime(lastMoveTime);
    
    if (restTime >= requiredRestTime) {
      isResting = false;
      Serial.println("\nReady for next command (Press 'M' for menu)");
    }
  }
}

void printMenu() {
  Serial.println("\n=== Servo Control Menu ===");
  Serial.println("P: Set Position (0-4095)");
  Serial.println("S: Set Speed (0-1023)");
  Serial.println("M: Show this menu");
  Serial.println("=====================");
  Serial.println("Operating at 35% duty cycle");
}