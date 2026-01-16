#include <Wire.h>

#define SDA_PIN 6
#define SCL_PIN 7
#define PIN_SDO D1
#define PIN_CS D0

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_SDO, OUTPUT);
  
  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_SDO, LOW);  // LOW = 0x68 || HIGH = 0x69

  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("I2C scan starting...");

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("I2C device found at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
    }
  }

  Serial.println("I2C scan done");
}

void loop() {}
