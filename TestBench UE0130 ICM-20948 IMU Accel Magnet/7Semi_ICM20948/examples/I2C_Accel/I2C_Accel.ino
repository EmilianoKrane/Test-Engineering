/***************************************************************
 * @file    I2C_Accel.ino
 * @brief   Minimal I2C bring-up for 7Semi ICM-20948 on UNO/ESP32 +
 *          continuous accelerometer readout 
 *
 * Features
 * - I2C init on default/custom pins
 * - IMU beginI2C() on I2C (addr 0x68/0x69)
 * - Manual sensor enable (setSensors)
 * - Accel config: scale, DLPF, averaging, sample rate
 * - Read accel (g) at ~2 Hz print
 *
 * Notes
 * - WHO_AM_I must be 0xEA
 * - Sensors must be explicitly enabled
 * - Bank switching handled internally by library
 *
 * Wiring (Arduino UNO I2C)
 * - SDA  -> A4
 * - SCL  -> A5
 * - VCC  -> 3V3 (or 5V if your board supports it)
 * - GND  -> GND
 *
 * Wiring (ESP32 default I2C)
 * - SDA  -> GPIO21
 * - SCL  -> GPIO22
 * - VCC  -> 3V3
 * - GND  -> GND
 ***************************************************************/

#include <Wire.h>
#include <7Semi_ICM20948.h>

/* ====================== User Config ======================= */
#define ICM_ADDR 0x69  // 0x68 default, change if AD0=HIGH

/** - IMU instance */
ICM20948_7Semi imu;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("ICM-20948 (I2C) — Accel Example"));

  /** Initialize IMU using I2C */
  if (!imu.beginI2C(ICM_ADDR, Wire, 400000)) {
    Serial.println(F("ERROR: ICM-20948 beginI2C() failed."));
    while (1) delay(200);
  }

  Serial.println(F("ICM-20948 ready."));

  /** Verify device identity */
  uint8_t who;
  if (imu.readWhoAmI(who)) {
    Serial.print("WHO_AM_I: 0x");
    Serial.println(who, HEX);
  }

  /** Enable only accelerometer
   * - accel = true
   * - gyro  = false
   * - temp  = false
   */
  if (!imu.setSensors(true, false, false)) {
    Serial.println(F("setSensors failed."));
  }

  /* ---------------- ACCEL CONFIG ----------------
   * - Scale: ±2/±4/±8/±16 g
   * - DLPF: noise filtering
   * - Averaging: noise reduction
   * - Sample rate: output frequency
   */

  /** Set accelerometer full-scale range */
  if (!imu.setAccelScale(ICM20948_Accel_FullScale::G_4)) {
    Serial.println(F("setAccelScale failed."));
  }

  /** Enable DLPF (recommended)
   * - bypass = false → DLPF enabled
   */
  if (!imu.setAccelDLPF(ACCEL_DLPFCFG_3, false)) {
    Serial.println(F("setAccelDLPF failed."));
  }

  /** Set averaging (noise reduction) */
  if (!imu.setAccelAveraging(ICM20948_Accel_Average::AVG_8)) {
    Serial.println(F("setAccelAveraging failed."));
  }

  /** Set output data rate (ODR)
   * - Base = 1125 Hz
   * - ODR = 1125 / (1 + divider)
   * - Example: 225 Hz
   */
  if (!imu.setAccelSampleRate(225)) {
    Serial.println(F("setAccelSampleRate failed."));
  }

  delay(10);
}

void loop() {
  float ax, ay, az;

  /** Read accelerometer data
   * - Output in g units
   * - Returns true on success
   */
  if (imu.readAccel(ax, ay, az)) {
    Serial.print("ACCEL [g]: ");
    Serial.print(ax, 3);
    Serial.print(", ");
    Serial.print(ay, 3);
    Serial.print(", ");
    Serial.println(az, 3);
  } else {
    Serial.println(F("ACCEL read failed"));
  }

  Serial.println(F("-----------------------------"));
  delay(500);
}