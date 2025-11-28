
#include <Arduino.h>
#include "ue_i2c_icp_10111_sen.h"
#include <Wire.h>

#define SDA_PIN 22
#define SCL_PIN 23

ICP101xx sensor;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize sensor.
  if (!sensor.begin(&Wire)) {
    Serial.println("ERROR: Could not initialize sensor!");
    Serial.println("Check I2C wiring and connections.");
    while (1) delay(1000);
  }
}

void loop() {
  sensor.measure(sensor.NORMAL);

  float pressure = sensor.getPressurePa();
  float temperature = sensor.getTemperatureC();

  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" Pa");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.println("");

  delay(1000);
}