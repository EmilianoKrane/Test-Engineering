#include <DevLab_ICM20948.h>

#define CS_PIN 18   // Chip Select (user pin)
#define SCK_PIN 6   // SPI SCK
#define MOSI_PIN 7  // SPI MOSI
#define MISO_PIN 2  // SPI MISO

/** IMU instance */
DevLab_ICM20948 imu;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("ICM-20948 — SPI Basic"));

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

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
    Serial.print(ax, 3);
    Serial.print(", ");
    Serial.print(ay, 3);
    Serial.print(", ");
    Serial.println(az, 3);
  } else {
    Serial.println(F("ACC read failed"));
  }

  /** Read Gyroscope */
  if (imu.readGyro(gx, gy, gz)) {
    Serial.print(F("GYR [dps]: "));
    Serial.print(gx, 2);
    Serial.print(", ");
    Serial.print(gy, 2);
    Serial.print(", ");
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