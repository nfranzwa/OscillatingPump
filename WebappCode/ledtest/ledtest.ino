#include <Arduino.h>

// Define the LED pins
const int greenLedPin = 4;  // Main LED (consider using a different pin if this doesn't work)
const int redLedPin = 0;     // Red LED

// Calibration state variable
int calibrationState = 1;  // Default state

// Variables for LED blinking without delays
unsigned long previousMillis = 0;
const long blinkInterval = 500;  // Blink interval in milliseconds (0.5 second)
int greenLedState = LOW;
int redLedState = LOW;

void setup() {
  // Initialize the LED pins as outputs
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  
  // Initialize Serial for debugging and input
  Serial.begin(115200);
  Serial.println("ESP32 Multi-LED Control Program Started");
  Serial.println("Enter calibration state (1-5):");
}

void loop() {
  // Check for serial input
  if (Serial.available() > 0) {
    // Read the incoming byte
    int newState = Serial.parseInt();
    
    // Consume any leftover characters (like newline)
    while (Serial.available() > 0) {
      Serial.read();
    }
    
    // Only update if we got a valid number
    if (newState > 0) {
      calibrationState = newState;
      Serial.print("Calibration state changed to: ");
      Serial.println(calibrationState);
      Serial.println("Enter calibration state (1-5):");
    }
  }
  
  // Get current time for non-blocking operations
  unsigned long currentMillis = millis();
  
  // Control LEDs based on calibration state
  if (calibrationState == 1) {
    // STATE 1: Blink the green LED
    digitalWrite(redLedPin, LOW);  // Make sure red LED is off
    
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      
      // Toggle green LED state
      greenLedState = (greenLedState == LOW) ? HIGH : LOW;
      digitalWrite(greenLedPin, greenLedState);
    }
  } 
  else if (calibrationState == 2 || calibrationState == 3) {
    // STATE 2 or 3: Green LED stays on
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(redLedPin, LOW);
  }
  else if (calibrationState == 4) {
    // STATE 4: Red LED on, Green LED off
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
  }
  else if (calibrationState == 5) {
    // STATE 5: Blink the red LED
    digitalWrite(greenLedPin, LOW);  // Make sure green LED is off
    
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      
      // Toggle red LED state
      redLedState = (redLedState == LOW) ? HIGH : LOW;
      digitalWrite(redLedPin, redLedState);
    }
  }
  else {
    // Default state: both LEDs off
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, LOW);
  }
  
  // Your other code can go here
  // This part of the code runs without being blocked
}