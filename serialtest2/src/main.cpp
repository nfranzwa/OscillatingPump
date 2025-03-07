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
float num1;
int num2;
float pmin;
float pmax;
float n1;
float n2;
float n3;
int n4;
int n5;
String serialInput; // this is for the computer serial
bool printVals=false;
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
    if(sscanf(msg.c_str(),"%f,%f,%f,%f,%f,%d,%d\n",
        &pmin,&pmax,&n1,&n2,&n3,&n4,&n5)){
      Serial.printf("S2: Pm:%-3.3f\tPM:%-3.3f\tn1: %-3.3f , n2: %-3.3f , n3: %-3.3f, n4: %d, n5: %d\n",pmin,pmax,n1,n2,n3,n4);
    };
    
    // Optionally, send the received data back to the Serial Monitor (via USB)
    // Serial.print("Echoing received data: ");
    // Serial.println(incomingData);
  }
  else{
    Serial.println("S2;nothing back");
  }
  if(Serial.available()){
    serialInput=Serial.readStringUntil('\n');
    //if there's a new message that matches, rewrite the variables
    if (sscanf(serialInput.c_str(), "%f,%d\n", &num1, &num2) == 2) {
        // Successfully parsed the message
        Serial.printf("Updated: num1 = %-4.4f,num2 = %d\n",num1,num2);
    }
    if (serialInput.startsWith("Toggle")){
        printVals=!printVals;
    }
  }
  // Optionally, send a debug message periodically to show the loop is running
  String dataToSend=(String) num1+","+(String)num2+"\n";
  const char* data = dataToSend.c_str();

  // Send the string using mySerial.write(), one byte at a time
  Serial1.write(reinterpret_cast<const uint8_t*>(data), dataToSend.length());
  // Serial1.printf("%f,%d\n",num1,num2);
  if(printVals) Serial.printf("S2; num1: %-4.4f, num2: %d\n",num1, num2);
  delay(200);
}
