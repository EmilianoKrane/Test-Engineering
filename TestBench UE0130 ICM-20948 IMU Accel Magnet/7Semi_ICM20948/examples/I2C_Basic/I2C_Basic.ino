/***************************************************************
 * @file    I2C_Basic.ino
 * @brief   Minimal I2C bring-up for 7Semi ICM-20948 +
 *          basic Accel/Gyro/Temp/Mag readout (updated API).
 *
 * Features
 * - I2C init (UNO / ESP32)
 * - beginI2C() initialization
 * - Manual sensor enable (setSensors)
 * - Read Accel / Gyro / Temp / Mag
 *
 * Notes
 * - WHO_AM_I must be 0xEA
 * - Sensors must be explicitly enabled
 * - Magnetometer requires initMag()
 *
 * Wiring (Arduino UNO I2C)
 * - SDA -> A4
 * - SCL -> A5
 * - VCC -> 3V3 (or 5V if supported)
 * - GND -> GND
 *
 * Wiring (ESP32 default I2C)
 * - SDA -> GPIO21
 * - SCL -> GPIO22
 ***************************************************************/
#include <7Semi_ICM20948.h>

/** User config */
#define ICM_ADDR 0x69  // 0x68 (AD0=LOW) or 0x69 (AD0=HIGH)

/** IMU instance */
ICM20948_7Semi imu;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("ICM-20948 — I2C Basic"));

  /** Initialize IMU */
  if (!imu.beginI2C(ICM_ADDR, Wire, 400000)) {
    Serial.println(F("ERROR: beginI2C() failed"));
    while (1) delay(200);
  }

  /** WHO_AM_I check */
  uint8_t who;
  if (!imu.readWhoAmI(who) || who != 0xEA) {
    Serial.println(F("ERROR: WHO_AM_I mismatch"));
    while (1) delay(200);
  }

  Serial.print(F("WHO_AM_I = 0x"));
  Serial.println(who, HEX);

  /** Enable all sensors */
  if (!imu.setSensors(true, true, true)) {
    Serial.println(F("ERROR: setSensors failed"));
  }

  /** Initialize magnetometer */
  if (!imu.initMag()) {
    Serial.println(F("Mag init failed"));
  } else {
    Serial.println(F("Mag initialized"));
  }

  Serial.println(F("Ready.\n"));
}

void loop() {
  float ax, ay, az;
  float gx, gy, gz;
  float mx, my, mz;
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

  /** Read Magnetometer */
  if (imu.readMag(mx, my, mz)) {
    Serial.print(F("MAG [uT]: "));
    Serial.print(mx, 2); Serial.print(", ");
    Serial.print(my, 2); Serial.print(", ");
    Serial.println(mz, 2);
  } else {
    Serial.println(F("MAG read failed"));
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

/* Note on magnetometer:
 * - Uses internal I2C master (AK09916 via ICM20948)
 * - initMag() must be called before readMag()
 * - If values are zero:
 *   → check initMag()
 *   → verify power + pull-ups
 */