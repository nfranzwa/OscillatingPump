#include <Arduino.h>

// Define RXD1 and TXD1 pins for serial communication
#define TXD1        18 // ESP serial communication
#define RXD1        19
// Define baud rate for communication
#define BAUD_RATE 115200

void setup() {
  // Start the serial communication with the baud rate of 115200
  Serial.begin(BAUD_RATE);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RXD1, TXD1);

  // Debug print to confirm setup
  Serial.println("Serial communication setup:");
  Serial.print("RXD1 pin: ");
  Serial.println(RXD1);
  Serial.print("TXD1 pin: ");
  Serial.println(TXD1);
  Serial.print("Baud Rate: ");
  Serial.println(BAUD_RATE);
}
float num1=1.0;
int num2=3;
void loop() {
  // Check if data is available to read from Serial1 (RXD1 and TXD1)
  if (Serial1.available()) {
    // Read incoming data from Serial1
    String msg = Serial1.readStringUntil('\n');  // Read until newline character
    // Debug print the received data
    Serial.print("S2 received: ");
    Serial.println(msg);
    if(sscanf(msg.c_str(),"%f,%d\n",&num1,&num2)){
      Serial.printf("S2: num1:%-3.3f\tnum2:%-3.3d\n",num1,num2);
    };
    // Optionally, send the received data back to the Serial Monitor (via USB)
    // Serial.print("Echoing received data: ");
    // Serial.println(incomingData);
  }
  else{
    Serial.println("S2;nothing back");
  }
  // Optionally, send a debug message periodically to show the loop is running
  Serial1.printf("%f,%d\n",num1,num2);
  delay(200);
}
