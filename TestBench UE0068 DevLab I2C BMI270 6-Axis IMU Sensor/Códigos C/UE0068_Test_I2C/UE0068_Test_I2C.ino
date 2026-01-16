#include <Wire.h>
#include "SparkFun_BMI270_Arduino_Library.h"

#define SDA_PIN 6
#define SCL_PIN 7
#define PIN_SDO D1
#define PIN_CS D0

// Create a new sensor object
BMI270 imu;

// I2C address selection
uint8_t addr_imu_1 = BMI2_I2C_PRIM_ADDR;  // 0x68
uint8_t addr_imu_2 = BMI2_I2C_SEC_ADDR; // 0x69

void setup() {
  // Start serial
  Serial.begin(115200);
  Serial.println("BMI270 Example 1 - Basic Readings I2C");

  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_SDO, OUTPUT);

  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_SDO, HIGH);  // LOW = 0x68 || HIGH = 0x69

  // Initialize the I2C library
  Wire.begin(SDA_PIN, SCL_PIN);

  bool imu_ok = false;

  Serial.println("Inicializando BMI270...");

  if (imu.beginI2C(addr_imu_1, Wire) == BMI2_OK) {
    Serial.println("BMI270 detectado en 0x68");
    imu_ok = true;
  } else if (imu.beginI2C(addr_imu_2, Wire) == BMI2_OK) {
    Serial.println("BMI270 detectado en 0x69");
    imu_ok = true;
  }

  while (!imu_ok) {
    Serial.println("Error: BMI270 no detectado en 0x68 ni 0x69");
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
  delay(20);
}