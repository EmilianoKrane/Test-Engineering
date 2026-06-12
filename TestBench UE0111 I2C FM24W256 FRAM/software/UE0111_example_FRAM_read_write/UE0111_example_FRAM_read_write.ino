#include <Wire.h>

// I2C configuration
#define SDA_PIN 6
#define SCL_PIN 7

#define FRAM_BASE 0x50
uint8_t framAddr = FRAM_BASE;

// Write function
bool framWrite(uint16_t addr, const uint8_t *data, size_t len) {
 Wire.beginTransmission(framAddr);
 Wire.write(addr >> 8);
 Wire.write(addr & 0xFF);
 Wire.write(data, len);
 return (Wire.endTransmission() == 0);
}

// Read function
bool framRead(uint16_t addr, uint8_t *buf, size_t len) {
 Wire.beginTransmission(framAddr);
 Wire.write(addr >> 8);
 Wire.write(addr & 0xFF);
 if (Wire.endTransmission(false) != 0) return false;

 if (Wire.requestFrom((int)framAddr, (int)len) != (int)len) return false;

 for (size_t i = 0; i < len; i++) {
   buf[i] = Wire.read();
 }
 return true;
}

// I2C scan
void i2cScan() {
 Serial.println("\nI2C scan:");
 for (uint8_t addr = 1; addr < 127; addr++) {
   Wire.beginTransmission(addr);
   if (Wire.endTransmission() == 0) {
     Serial.printf("Device found at 0x%02X\n", addr);
     if (addr >= 0x50 && addr <= 0x57) {
       framAddr = addr;
     }
   }
 }
}

void setup() {
 Serial.begin(115200);

#if defined(ESP8266) || defined(ESP32)
 Wire.begin(SDA_PIN, SCL_PIN);
#else
 Wire.begin();
#endif

 Wire.setClock(100000);

 i2cScan();

 const char* data = "Hola FRAM";
 uint16_t addr = 0x0010;

 framWrite(addr, (const uint8_t*)data, strlen(data));

 uint8_t buffer[32] = {0};
 framRead(addr, buffer, strlen(data));

 Serial.print("Read: ");
 Serial.println((char*)buffer);
}

void loop() {}