#include <SPI.h>
#include "DevLab_BMI323.h"

// *************Setup SPI Config
#define CS_PIN 18   // Chip Select CS
#define SCK_PIN 6   // SPI SCK  / I2C SCL
#define MOSI_PIN 7  // SPI MOSI / I2C SDAs
#define MISO_PIN 2  // SPI MISO SDO ADO SAO
#define SPI_FAST_SPEED 3000000

// CORRECCIÓN 1: Elimina "SPIClass spi_bus(SPI);" y pasa el objeto global "SPI" directamente.
DevLab_BMI323 imuSpi(SPI, CS_PIN, MISO_PIN, MOSI_PIN, SCK_PIN, SPI_FAST_SPEED);

BMI323_SensorData data;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando puerto");

  // CORRECCIÓN 2: Preparar el pin CS manualmente.
  // Los sensores Bosch arrancan en modo I2C por defecto. Para que cambien a modo SPI,
  // necesitan detectar obligatoriamente que el pin CS pasa de estado ALTO a BAJO.
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  delay(100);  // Breve espera para estabilizar el voltaje

  // CORRECCIÓN 3: Pasar -1 en el argumento del CS para evitar que
  // el controlador SPI de hardware secuestre el pin.
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, -1);

  if (!imuSpi.begin()) {
    while (1) {
      Serial.println("Error : BMI323 initialization failed");
      delay(1000);
    }
  } else {
    Serial.println("BMI323 Initialized succesfully");
  }

  delay(10);

  // Asegúrate de que los macros BMI323_CHIP_ID y REG_CHIP_ID estén definidos en tu .h
  imuSpi.test_chip_id(BMI323_CHIP_ID, REG_CHIP_ID);
}

void loop() {
  // Read sensor data
  if (imuSpi.readData(data)) {
    Serial.println("--------------------------------------------------");

    // Accelerometer data
    Serial.print("Accelerometer [raw]");
    Serial.print("  X: ");
    Serial.print(data.accX);

    Serial.print("   Y: ");
    Serial.print(data.accY);

    Serial.print("   Z: ");
    Serial.println(data.accZ);

    // Gyroscope data
    Serial.print("Gyroscope     [raw]");
    Serial.print("  X: ");
    Serial.print(data.gyrX);

    Serial.print("   Y: ");
    Serial.print(data.gyrY);

    Serial.print("   Z: ");
    Serial.println(data.gyrZ);

    // Temperature data
    Serial.print("Temperature   [C]");
    Serial.print("    ");
    Serial.println(data.temperatureC, 2);

  } else {
    Serial.println("ERROR: Failed to read BMI323 data.");
  }

  delay(100);
}