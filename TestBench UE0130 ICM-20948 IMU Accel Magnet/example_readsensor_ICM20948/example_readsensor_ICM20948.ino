// Este firmware requiere de la actualización de la libreria local ya que descarga una versión vieja


#include <7Semi_ICM20948.h>
#include <Wire.h>
#include <Arduino.h>

// ==== Declaraciones ====
#define ICM_ADDR 0x69  // 0x68 (AD0=LOW) or 0x69 Default (AD0=HIGH)
#define SDA_PIN 6
#define SCL_PIN 7

// ==== Creación de Objetos ====
ICM20948_7Semi imu;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Pequeño delay extra para que el puerto serial del ESP32 levante bien
  Serial.println("\nSerial Initialized...");

  // Inicializa I2C en los pines correctos para tu ESP32-C6
  Wire.begin(SDA_PIN, SCL_PIN);

  // ==== Inicialización del IMU ====
  // CORRECCIÓN: Se pasa 'Wire' directamente, no '&Wire'
  if (!imu.beginI2C(ICM_ADDR, Wire, 400000)) {
    Serial.println(F("ERROR: No se encuentra el sensor (revisa pines o dirección)"));
    while (1) delay(200);
  }

  Serial.println(F("ICM20948 detectado correctamente!"));

  // WHO_AM_I check (Descomentado y corregido)
  uint8_t who;
  if (imu.readWhoAmI(who)) {
    Serial.print(F("WHO_AM_I = 0x"));
    Serial.println(who, HEX);  // Debería imprimir 0xEA
  } else {
    Serial.println(F("Fallo al leer WHO_AM_I"));
  }

  // Habilitar todos los sensores
  if (!imu.setSensors(true, true, true)) {
    Serial.println(F("ERROR: setSensors failed"));
  }

  // Inicializar Magnetómetro
  if (!imu.initMag()) {
    Serial.println(F("Fallo al inicializar Magnetómetro (AK09916)"));
  } else {
    Serial.println(F("Magnetómetro listo"));
  }

  // imu.setMagOpMode(ICM20948_Op_Mode::MODE_CONTINUOUS_4);

  Serial.println(F("Muestreando datos...\n"));
}

void loop() {
  float ax, ay, az, gx, gy, gz, mx, my, mz, tC;

  // Lecturas de Acelerómetro
  if (imu.readAccel(ax, ay, az)) {
    Serial.print(F("ACC (g): "));
    Serial.print(ax);
    Serial.print(", ");
    Serial.print(ay);
    Serial.print(", ");
    Serial.println(az);
  }

  if (imu.readGyro(gx, gy, gz)) {
    Serial.print(F("GYR (g): "));
    Serial.print(gx);
    Serial.print(", ");
    Serial.print(gy);
    Serial.print(", ");
    Serial.println(gz);
  }

  if (imu.readTemperature(tC)) {
    Serial.print("Temp: ");
    Serial.println(tC);
  }

  // Lecturas de Magnetómetro
  if (imu.readMag(mx, my, mz)) {
    Serial.print(F("MAG (uT): "));
    Serial.print(mx);
    Serial.print(", ");
    Serial.print(my);
    Serial.print(", ");
    Serial.println(mz);
  }

  uint8_t check;
  if (!imu.readWhoAmI(check) || check != 0xEA) {
    Serial.println("¡ALERTA! Comunicación perdida con el sensor. Revisa cableado.");
  }

  Serial.println("---");
  delay(500);
}