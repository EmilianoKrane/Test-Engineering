#include <SPI.h>
#include "SparkFun_BMI270_Arduino_Library.h"

#define SDA_PIN 6   // MOSI
#define SCL_PIN 7   // SCl
#define PIN_SDO D1  // MISO
#define PIN_CS D0

// Create a new sensor object
BMI270 imu;

// SPI parameters
uint8_t chipSelectPin = D0;
uint32_t clockFrequency = 100000;

void setup() {
  // Start serial
  Serial.begin(115200);
  Serial.println("BMI270 Example 2 - Basic Readings SPI");

  pinMode(PIN_CS, INPUT);
  pinMode(PIN_SDO, OUTPUT);

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


  digitalWrite(PIN_SDO, HIGH);  // LOW = 0x68 || HIGH = 0x69

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
  delay(10000);

  // Initialize the SPI library
  // void begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1);
  SPI.begin(7, D1, 6, D0);
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, LOW);

  // Check if sensor is connected and initialize
  // Clock frequency is optional (defaults to 100kHz)
  while (imu.beginSPI(chipSelectPin, clockFrequency) != BMI2_OK) {
    // Not connected, inform user
    Serial.println("Error: BMI270 not connected, check wiring and CS pin!");

    // Wait a bit to see if connection is established
    delay(1000);
  }

  Serial.println("BMI270 connected!");
}

void loop() {
  // Get measurements from the sensor. This must be called before accessing
  // the sensor data, otherwise it will never update
  imu.getSensorData();

  // Print acceleration data
  Serial.print("Acceleration in g's");
  Serial.print("\t");
  Serial.print("X: ");
  Serial.print(imu.data.accelX, 3);
  Serial.print("\t");
  Serial.print("Y: ");
  Serial.print(imu.data.accelY, 3);
  Serial.print("\t");
  Serial.print("Z: ");
  Serial.print(imu.data.accelZ, 3);

  Serial.print("\t");

  // Print rotation data
  Serial.print("Rotation in deg/sec");
  Serial.print("\t");
  Serial.print("X: ");
  Serial.print(imu.data.gyroX, 3);
  Serial.print("\t");
  Serial.print("Y: ");
  Serial.print(imu.data.gyroY, 3);
  Serial.print("\t");
  Serial.print("Z: ");
  Serial.println(imu.data.gyroZ, 3);

  // Print 50x per second
  delay(100);
}