#include <PA12.h>

#define ID_NUM 0  // ID of the actuator
PA12 myServo(&Serial1, 2, 1);  // Initialize the PA12 object

// Oscillation parameters
float minPosition = 0;
float maxPosition = 4095;
float frequency = 0.5;  // Default frequency in Hz
bool isOscillating = false;
unsigned long previousMillis = 0;
float currentPosition = 0;

void setup() {
  Serial.begin(9600);  // Initialize serial communication
  myServo.begin(32);   // Initialize the PA12 bus with a baudrate of 57600
  Serial.println("Ready to receive commands. Use 'MIN: x', 'MAX: y', 'FREQ: z' to set oscillation parameters.");
  Serial.println("Use 'START' to begin oscillation and 'STOP' to stop.");
}

void loop() {
  if (isOscillating) {
    unsigned long currentMillis = millis();
    float period = 1000.0 / frequency;  // Period in milliseconds

    // Calculate the time fraction within the current period
    float timeFraction = (currentMillis % (unsigned long)period) / period;

    // Implement a 40% duty cycle
    if (timeFraction < 0.4) {
      currentPosition = maxPosition;  // 40% of the time at maxPosition
    } else {
      currentPosition = minPosition;  // 60% of the time at minPosition
    }

    // Move the actuator to the calculated position
    myServo.goalPosition(ID_NUM, (int)currentPosition);

    // Optional: Print current position for debugging
    Serial.print("Current Position: ");
    Serial.println(currentPosition);
  }

  // Handle serial commands
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("MIN:")) {
      minPosition = input.substring(4).toFloat();
      minPosition = constrain(minPosition, 0, 4095);
      Serial.print("Set MIN position to: ");
      Serial.println(minPosition);
    } else if (input.startsWith("MAX:")) {
      maxPosition = input.substring(4).toFloat();
      maxPosition = constrain(maxPosition, 0, 4095);
      Serial.print("Set MAX position to: ");
      Serial.println(maxPosition);
    } else if (input.startsWith("FREQ:")) {
      frequency = input.substring(5).toFloat();
      frequency = constrain(frequency, 0.1, 10.0);  // Limit frequency to a reasonable range
      Serial.print("Set frequency to: ");
      Serial.println(frequency);
    } else if (input.equalsIgnoreCase("START")) {
      isOscillating = true;
      Serial.println("Oscillation started.");
    } else if (input.equalsIgnoreCase("STOP")) {
      isOscillating = false;
      Serial.println("Oscillation stopped.");
    } else {
      Serial.println("Invalid command. Use 'MIN: x', 'MAX: y', 'FREQ: z', 'START', or 'STOP'.");
    }
  }
}