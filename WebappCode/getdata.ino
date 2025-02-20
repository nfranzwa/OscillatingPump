#include <Wire.h>

std::vector<uint16_t> myList = {};
#define I2C_ADDRESS 0x28  // I2C address of the pressure sensor
float maptopsi(int value){
  if (value < -16383) value = -16383;
  if (value > 16383)  value = 16383;
  // Perform the mapping:
  // We need to scale from a 32766 range (-16383 to 16383) to a 200 range (-100 to 100)
  float result = (value / 16383.0) * 100.0;
  result = result * 0.0145;
  return result;
}
String readPressureRaw() {
  int pressure_raw = 0;
  Wire.requestFrom(I2C_ADDRESS, 2);
  if (Wire.available() >= 2) {
    uint8_t highByte = Wire.read();
    uint8_t lowByte = Wire.read();
    pressure_raw = (highByte << 8) | lowByte;
  } else {
    Serial.println("No data received.");
  }
  float psi = maptopsi(pressure_raw);
  //Serial.println(psi);
  String pressure = String(psi, 3);
  return pressure;
}
void setup() {
  Serial.begin(115200);
  Wire.begin();  // Initialize I2C (SDA: GPIO21, SCL: GPIO22)

  // Check if the sensor is responding
  Wire.beginTransmission(I2C_ADDRESS);
  if (Wire.endTransmission() == 0) {
    Serial.println("Pressure sensor found at 0x28.");
  } else {
    Serial.println("Pressure sensor not found! Check wiring.");
  }
}

void loop() {
  String pres = readPressureRaw();
  Serial.println(pres);
  delay(100);
}
