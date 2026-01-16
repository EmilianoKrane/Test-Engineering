/*
Código principal de UE0068 
*/


#include <Wire.h>
#include <SPI.h>
#include "SparkFun_BMI270_Arduino_Library.h"

#define SDA_PIN 6
#define SCL_PIN 7
#define MOSI_PIN 6
#define MISO_PIN D1
#define SCK_PIN 7

#define SDO_PIN D1
#define CS_PIN D0

uint8_t addr_imu_1 = BMI2_I2C_PRIM_ADDR;  // 0x68
uint8_t addr_imu_2 = BMI2_I2C_SEC_ADDR;   // 0x69

char cmd = 'a';          // 'a' = I2C 0x68 | 'b' = I2C 0x69 | 'c' = SPI
char last_cmd = 0;
bool imu_ok = false;

BMI270 imu;


void setup() {
  Serial.begin(115200);

  pinMode(CS_PIN, OUTPUT);
  pinMode(SDO_PIN, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

  digitalWrite(CS_PIN, HIGH);  // Default I2C

  Serial.println("BMI270 Hybrid I2C / SPI");
}


void loop() {

  // Leer comando por Serial
  if (Serial.available()) {
    cmd = Serial.read();
  }

  // Si cambia el comando → reinicializar
  if (cmd != last_cmd) {
    initIMU(cmd);
    last_cmd = cmd;
    delay(100);
  }

  if (!imu_ok) return;

  imu.getSensorData();

  Serial.print("A[g] ");
  Serial.print(imu.data.accelX, 3);
  Serial.print(", ");
  Serial.print(imu.data.accelY, 3);
  Serial.print(", ");
  Serial.print(imu.data.accelZ, 3);

  Serial.print(" | G[dps] ");
  Serial.print(imu.data.gyroX, 3);
  Serial.print(", ");
  Serial.print(imu.data.gyroY, 3);
  Serial.print(", ");
  Serial.println(imu.data.gyroZ, 3);

  delay(50);
}




void initIMU(char mode) {
  imu_ok = false;

  if (mode == 'a') {
    Serial.println("Modo I2C 0x68");
    digitalWrite(CS_PIN, HIGH);
    digitalWrite(SDO_PIN, LOW);

    if (imu.beginI2C(addr_imu_1, Wire) == BMI2_OK) {
      imu_ok = true;
    }
  }

  else if (mode == 'b') {
    Serial.println("Modo I2C 0x69");
    digitalWrite(CS_PIN, HIGH);
    digitalWrite(SDO_PIN, HIGH);

    if (imu.beginI2C(addr_imu_2, Wire) == BMI2_OK) {
      imu_ok = true;
    }
  }

  else if (mode == 'c') {
    Serial.println("Modo SPI");
    digitalWrite(CS_PIN, LOW);

    if (imu.beginSPI(CS_PIN, 100000) == BMI2_OK) {
      imu_ok = true;
    }
  }

  if (!imu_ok) {
    Serial.println("Error inicializando BMI270");
  }
}
