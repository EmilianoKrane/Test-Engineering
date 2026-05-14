/***************************************************************
 * @file    SPI_Basic.ino
 * @brief   Minimal SPI bring-up for 7Semi ICM-20948 +
 *          basic Accel/Gyro/Temp readout (updated API).
 *
 * Features
 * - SPI init (UNO / ESP32)
 * - beginSPI() initialization
 * - Manual sensor enable
 * - Read Accel / Gyro / Temp
 *
 * Notes
 * - WHO_AM_I must be 0xEA
 * - Sensors must be explicitly enabled
 * - SPI speed ~1MHz recommended during init
 *
 * Wiring (Arduino UNO SPI)
 * - SCK  -> D13
 * - MOSI -> D11
 * - MISO -> D12
 * - CS   -> D10 (changeable; update CS_PIN)
 * - VCC  -> 3V3
 * - GND  -> GND
 *
 * Wiring (ESP32 VSPI default)
 * - SCK  -> GPIO18
 * - MOSI -> GPIO23
 * - MISO -> GPIO19
 * - CS   -> GPIO5
 ***************************************************************/
#include <7Semi_ICM20948.h>

static const uint8_t CS_PIN   = 10;

/** IMU instance */
ICM20948_7Semi imu;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("ICM-20948 — SPI Basic"));


  /** Initialize IMU using SPI */
  if (!imu.beginSPI(CS_PIN, SPI, 1000000)) {
    Serial.println(F("ERROR: beginSPI() failed"));
    while (1) delay(200);
  }

  /** Enable all sensors
   * - accel = true
   * - gyro  = true
   * - temp  = true
   */
  if (!imu.setSensors(true, true, true)) {
    Serial.println(F("ERROR: setSensors failed"));
  }

  Serial.println(F("Ready.\n"));
}

void loop() {
  float ax, ay, az;
  float gx, gy, gz;
  float tC;

  /** Read Accelerometer */
  if (imu.readAccel(ax, ay, az)) {
    Serial.print(F("ACC [g]: "));
    Serial.print(ax, 3); Serial.print(", ");
    Serial.print(ay, 3); Serial.print(", ");
    Serial.println(az, 3);
  } else {
    Serial.println(F("ACC read failed"));
  }

  /** Read Gyroscope */
  if (imu.readGyro(gx, gy, gz)) {
    Serial.print(F("GYR [dps]: "));
    Serial.print(gx, 2); Serial.print(", ");
    Serial.print(gy, 2); Serial.print(", ");
    Serial.println(gz, 2);
  } else {
    Serial.println(F("GYR read failed"));
  }

  /** Read Temperature */
  if (imu.readTemperature(tC)) {
    Serial.print(F("TMP [C]: "));
    Serial.println(tC, 2);
  } else {
    Serial.println(F("TMP read failed"));
  }

  Serial.println(F("-----------------------------"));
  delay(500);
}
