/*
=== MainCode JSON UE0109 BMA255 ===
Código de integración para BMA255 con lectura de I2C y SPI que debe ser
usado con una pulsar c6 por medio de su puerto COM nativo
*/

#include <Wire.h>
#include <Arduino.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "BMA250.h"


// ==== DECLARACIÓN DE PINES ====
#define RUN_BUTTON 4  // Botón de Arranque
#define SDA_PIN 6     // >> GPIO06 MOSI
#define SCL_PIN 7     // >> GPIO07 SCL
#define SDO_PIN 2     // >> GPIO02 MISO
#define CS_PIN 18     // >> GPIO18 CS
#define PS_PIN 21     // >> GPIO21 PS


// ==== CREACIÓN DE OBJETOS ====
UBMA250 accel_sensor;

String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<300> receiveJSON;

String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<300> sendJSON;

// ==== DECLARACIÓN DE VARIABLES GLOBALES ====
int x, y, z;
double temp;


// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  StaticJsonDocument<200> debugDoc;
  debugDoc["debug"] = str;
  serializeJson(debugDoc, Serial);
  Serial.println();
}

void showSerial() {
  Serial.print("X = ");
  Serial.print(x);

  Serial.print("  Y = ");
  Serial.print(y);

  Serial.print("  Z = ");
  Serial.print(z);

  Serial.print("  Temperature(C) = ");
  Serial.println(temp);
}

void showJSON() {
  sendJSON["accelX"] = x;
  sendJSON["accelY"] = y;
  sendJSON["accelZ"] = z;
}


void setup() {

  // ==== Inicialización de UART ====
  Serial.begin(115200);

  // ==== Declaración de pines de entrada y salida ====
  pinMode(SDO_PIN, OUTPUT);
  pinMode(PS_PIN, OUTPUT);
}

void loop() {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {
      String Function = receiveJSON["Function"];  // Function es la variable de interés del JSON

      int opc = 0;

      if (Function == "ping") opc = 1;        // {"Function":"ping"}
      else if (Function == "i2c18") opc = 2;  // {"Function":"i2c18"}
      else if (Function == "i2c19") opc = 3;  // {"Function":"i2c19"}
      else if (Function == "spi") opc = 4;    // {"Function":"spi"}

      switch (opc) {

        // Validación de comunicación UART con la PagWeb <-> Pulsar
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // ---- Lectura de sensor con I2C 0x18 ----
        case 2:
          {
            sendJSON.clear();
            serialDebug("Test I2C Addr 0x18 Initialized...");

            SPI.end();                 // <-- 1. CIERRA EL BUS SPI
            pinMode(SDO_PIN, OUTPUT);  // <-- 2. ASEGURA QUE SDO SEA SALIDA PARA LA DIRECCIÓN

            digitalWrite(PS_PIN, HIGH);
            digitalWrite(SDO_PIN, LOW);
            delay(100);

            Wire.begin(SDA_PIN, SCL_PIN);
            int result = -1, step = 15;

            for (int i = 0; i < 3; i++) {
              result = accel_sensor.begin(BMA250_range_2g, BMA250_update_time_64ms);
              delay(50);
              if (result == 0) {
                Serial.print("Addr I2C: ");
                Serial.println(accel_sensor.I2Caddress, HEX);
                break;
              } else {
                sendJSON.clear();
                sendJSON["i2c0x18"] = "Fail";
                serializeJson(sendJSON, Serial);
                Serial.println();
                break;
              }
              delay(100);
            }

            if (result == 0) {
              for (int j = 0; j < step; j++) {
                accel_sensor.read();
                x = accel_sensor.X;
                y = accel_sensor.Y;
                z = accel_sensor.Z;
                //temp = ((accel_sensor.rawTemp * 0.5) + 24.0);

                // Check if the BMA250 is not found or connected correctly
                if (x != -1 && y != -1 && z != -1) {
                  sendJSON.clear();
                  sendJSON["i2c0x18"] = "OK";
                  sendJSON["addr"] = String(accel_sensor.I2Caddress, HEX);
                  showJSON();
                  serializeJson(sendJSON, Serial);
                  Serial.println();
                  delay(100);
                }
              }
            }

            Serial.println("Acabo i2c");
            break;
          }

        // ---- Lectura de sensor con I2C 0x19 ----
        case 3:
          {
            sendJSON.clear();
            serialDebug("Test I2C Addr 0x19 Initialized...");

            SPI.end();                 // <-- 1. CIERRA EL BUS SPI
            pinMode(SDO_PIN, OUTPUT);  // <-- 2. ASEGURA QUE SDO SEA SALIDA PARA LA DIRECCIÓN

            digitalWrite(PS_PIN, HIGH);
            digitalWrite(SDO_PIN, HIGH);  // (o HIGH para el caso 3)
            delay(100);

            Wire.begin(SDA_PIN, SCL_PIN);
            int result = 10;

            for (int i = 0; i < 10; i++) {
              result = accel_sensor.begin(BMA250_range_2g, BMA250_update_time_64ms);
              delay(50);
              if (result == 0) {
                Serial.print("Addr I2C: ");
                Serial.println(accel_sensor.I2Caddress, HEX);
                break;
              } else {
                sendJSON.clear();
                sendJSON["i2c0x19"] = "Fail";
                serializeJson(sendJSON, Serial);
                Serial.println();
                break;
              }
              delay(100);
            }

            if (result == 0) {
              for (int j = 0; j < 20; j++) {
                accel_sensor.read();
                x = accel_sensor.X;
                y = accel_sensor.Y;
                z = accel_sensor.Z;
                //temp = ((accel_sensor.rawTemp * 0.5) + 24.0);

                // Check if the BMA250 is not found or connected correctly
                if (x != -1 && y != -1 && z != -1) {
                  sendJSON.clear();
                  sendJSON["i2c0x19"] = "OK";
                  sendJSON["addr"] = String(accel_sensor.I2Caddress, HEX);
                  showJSON();
                  serializeJson(sendJSON, Serial);
                  Serial.println();
                  delay(100);
                }
              }
            }

            Serial.println("Acabo i2c");
            break;
          }

        case 4:
          {
            sendJSON.clear();

            Wire.end();               // <-- 1. CIERRA EL BUS I2C
            pinMode(SDO_PIN, INPUT);  // <-- 2. SDO AHORA ES MISO (ENTRADA DE DATOS)

            digitalWrite(PS_PIN, LOW);  // Selector de protocolo a SPI
            delay(100);

            SPI.begin(SCL_PIN, SDO_PIN, SDA_PIN, CS_PIN);
            int result = accel_sensor.beginSPI(BMA250_range_2g, BMA250_update_time_64ms, CS_PIN, &SPI);

            if (result == 0) {  // Correcta inicialización del sensor
              for (int j = 0; j < 15; j++) {
                accel_sensor.read();
                x = accel_sensor.X;
                y = accel_sensor.Y;
                z = accel_sensor.Z;
                temp = ((accel_sensor.rawTemp * 0.5) + 24.0);

                if (x != -1 && y != -1 && z != -1) {
                  sendJSON.clear();
                  sendJSON["SPI"] = "OK";
                  showJSON();
                  serializeJson(sendJSON, Serial);
                  Serial.println();
                  delay(100);
                }
              }
            } else {
              sendJSON.clear();
              sendJSON["SPI"] = "Fail";
              serializeJson(sendJSON, Serial);
              Serial.println();
            }

            break;
          }
      }
    }
  }


  delay(100);
}
