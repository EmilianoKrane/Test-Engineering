#include <Wire.h>

#define SDA_PIN 6
#define SCL_PIN 7
#define PIN_SDO D1
#define PIN_CS D0

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setTimeout(5000);
  Serial.println("I2C scan starting...");

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("I2C device found at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
    }
  }

  Serial.println("I2C first scan done");
}

void loop() {

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');

    if (cmd = "r") {
      for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
          Serial.print("I2C device found at 0x");
          if (addr < 16) Serial.print("0");
          Serial.println(addr, HEX);
        }
      }
    }
  }
}
