#define ZERO_SAMPLES 50
#define COUNTS_PER_PSI 735990  // Increased to further scale down PSI values
#define PSI_TO_CMH2O 70.307    // Conversion factor from PSI to cmH2O
#define DRIFT_THRESHOLD 0.1    // PSI threshold to consider as "no pressure"
#define STABLE_TIME 3000       // Time in ms that readings need to be stable for re-zeroing
#define DRIFT_SAMPLES 10       // Number of samples to check for drift

long zeroOffset = 0;
bool isCalibrated = false;
int sampleCount = 0;
long calibrationSum = 0;
// Drift compensation variables
unsigned long stableStartTime = 0;
bool isStable = false;
float lastPSI = 0;
int stableSampleCount = 0;

void setup() {
  pinMode(2, INPUT);   // HX710 OUT
  pinMode(3, OUTPUT);  // HX710 SCK
  Serial.begin(9600);
  
  // Initialize sensor - power cycle the clock line
  digitalWrite(3, LOW);
  delay(100);
  for(int i = 0; i < 10; i++) {
    digitalWrite(3, HIGH);
    delayMicroseconds(100);
    digitalWrite(3, LOW);
    delayMicroseconds(100);
  }
  
}

long readPressureRaw() {
  while (digitalRead(2)) {}
  delayMicroseconds(100);

  long result = 0;
  noInterrupts();
  
  for (int i = 0; i < 24; i++) {
    digitalWrite(3, HIGH);
    delayMicroseconds(10);
    digitalWrite(3, LOW);
    delayMicroseconds(10);
    result = (result << 1);
    if (digitalRead(2)) {
      result++;
    }
  }
  
  for (char i = 0; i < 3; i++) {
    digitalWrite(3, HIGH);
    delayMicroseconds(10);
    digitalWrite(3, LOW);
    delayMicroseconds(10);
  }
  
  interrupts();
  
  return result ^ 0x800000;
}

float convertToPSI(long rawValue) {
  return (float)(rawValue - zeroOffset) / COUNTS_PER_PSI;
}

float convertToCmH2O(float psi) {
  return psi * PSI_TO_CMH2O;
}

void checkAndUpdateZero(long rawPressure, float currentPSI) {
  // Check if current pressure is close to zero
  if (abs(currentPSI - 1) < DRIFT_THRESHOLD) {  // Remember we're applying -1 offset to PSI
    if (!isStable) {
      // Start tracking stable period
      isStable = true;
      stableStartTime = millis();
      stableSampleCount = 1;
    } else {
      stableSampleCount++;
      
      // Check if we've been stable long enough and have enough samples
      if (stableSampleCount >= DRIFT_SAMPLES && 
          (millis() - stableStartTime) >= STABLE_TIME) {
        // Update zero offset
        zeroOffset = rawPressure;
        stableSampleCount = 0;
        isStable = false;
      }
    }
  } else {
    // Reset stable tracking if pressure is not near zero
    isStable = false;
    stableSampleCount = 0;
  }
}

void loop() {
    long pressure = readPressureRaw();
    

    if (!isCalibrated) {
      calibrationSum += pressure;
      sampleCount++;

      if (sampleCount >= ZERO_SAMPLES) {
        zeroOffset = calibrationSum / ZERO_SAMPLES;
        isCalibrated = true;
      }
    }
    else {
      float psi = convertToPSI(pressure);
      float cmH2O = convertToCmH2O(psi);  // Apply same offset as PSI
      
      // Check for drift and update zero if necessary
      checkAndUpdateZero(pressure, psi);
      
      Serial.print(pressure);
      Serial.print(",");
      Serial.print(psi, 3);  // Display with 4 decimal places
      Serial.print(",");
      Serial.print(cmH2O, 3);  // Display with 4 decimal places
      Serial.println(",");
      lastPSI = psi;
    }
    
    delay(100);
  
}
