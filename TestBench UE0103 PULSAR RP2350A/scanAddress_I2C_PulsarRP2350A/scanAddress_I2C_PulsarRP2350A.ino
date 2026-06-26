#include <Wire.h>

#define SDA_PIN 24
#define SCL_PIN 25

void setup() {
  Serial.begin(115200);
  
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();
  
  Serial.println("Escaneando I2C...");
  
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Dispositivo en 0x");
      Serial.println(addr, HEX);
    }
    delay(10);
  }
  
  Serial.println("Scan completo");
}

void loop() {}