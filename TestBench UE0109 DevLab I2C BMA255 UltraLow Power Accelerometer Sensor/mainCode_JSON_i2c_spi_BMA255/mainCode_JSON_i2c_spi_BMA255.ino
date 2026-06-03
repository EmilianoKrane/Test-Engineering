/**
 * @file      MainCode_JSON_UE0109_BMA255.ino
 * @brief     Firmware de integración para sensor acelerómetro BMA255.
 * Control de lectura I2C y SPI controlado vía comandos JSON por puerto COM.
 * @author    EmilianoKrane
 * @company   UNIT Electronics
 * @hardware  Pulsar C6 (ESP32-C6)
 */

#include <Wire.h>
#include <Arduino.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "BMA250.h"

// ==========================================
// ==== DECLARACIÓN DE PINES ================
// ==========================================
#define RUN_BUTTON 4  // Botón de Arranque
#define SDA_PIN 6     // >> GPIO06 MOSI / SDA
#define SCL_PIN 7     // >> GPIO07 SCL / SCK
#define SDO_PIN 2     // >> GPIO02 MISO / I2C Addr Selector
#define CS_PIN 18     // >> GPIO18 CS
#define PS_PIN 21     // >> GPIO21 PS (Selector de Protocolo)

// ==========================================
// ==== CREACIÓN DE OBJETOS Y GLOBALES ======
// ==========================================
UBMA250 accel_sensor;

String JSON_entrada;  // Variable que recibe el JSON en crudo desde la Web
StaticJsonDocument<300> receiveJSON;

StaticJsonDocument<300> sendJSON;  // Objeto para construir el JSON de salida

int x, y, z;
double temp;

// ==========================================
// ==== FUNCIONES DE UTILIDAD ===============
// ==========================================

/**
 * @brief Envía mensajes de depuración en formato JSON válido.
 * @param str Mensaje de texto a enviar.
 */
void serialDebug(String str) {
  StaticJsonDocument<200> debugDoc;
  debugDoc["debug"] = str;
  serializeJson(debugDoc, Serial);
  Serial.println();
}

/**
 * @brief Imprime los datos del sensor en formato de texto plano.
 * @note  Uso exclusivo para depuración manual, evitar si la web espera solo JSON.
 */
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

/**
 * @brief Agrega las coordenadas del acelerómetro al JSON de salida actual.
 */
void showJSON() {
  sendJSON["accelX"] = x;
  sendJSON["accelY"] = y;
  sendJSON["accelZ"] = z;
}

// ==========================================
// ==== CONFIGURACIÓN INICIAL (SETUP) =======
// ==========================================
void setup() {
  // Inicialización de UART para comunicación con frontend
  Serial.begin(115200);

  // Configuración de pines de control de protocolo
  pinMode(SDO_PIN, OUTPUT);
  pinMode(PS_PIN, OUTPUT);
}

// ==========================================
// ==== BUCLE PRINCIPAL (LOOP) ==============
// ==========================================
void loop() {
  // Verifica si hay comandos entrantes desde la página web
  if (Serial.available()) {
    JSON_entrada = Serial.readStringUntil('\n');  // Leer hasta newline

    // Deserializa el JSON entrante
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];  // Extracción del comando

      int opc = 0;
      if (Function == "ping") opc = 1;        // {"Function":"ping"}
      else if (Function == "i2c18") opc = 2;  // {"Function":"i2c18"}
      else if (Function == "i2c19") opc = 3;  // {"Function":"i2c19"}
      else if (Function == "spi") opc = 4;    // {"Function":"spi"}

      switch (opc) {

        // ---------------------------------------------------------
        // CASO 1: Validación de comunicación (Ping-Pong)
        // ---------------------------------------------------------
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // ---------------------------------------------------------
        // CASO 2: Lectura de sensor vía I2C (Dirección 0x18)
        // ---------------------------------------------------------
        case 2:
          {
            sendJSON.clear();
            serialDebug("Test I2C Addr 0x18 Initialized...");

            // 1. Preparación de Buses: Cierra SPI y configura pines para I2C
            SPI.end();
            pinMode(SDO_PIN, OUTPUT);

            // 2. Configuración de Hardware: Modo I2C y Dirección 0x18
            digitalWrite(PS_PIN, HIGH);
            digitalWrite(SDO_PIN, LOW);
            delay(100);  // Tiempo de estabilización del hardware

            Wire.begin(SDA_PIN, SCL_PIN);
            int result = -1, step = 15;

            // 3. Intento de inicialización del sensor
            for (int i = 0; i < 3; i++) {
              result = accel_sensor.begin(BMA250_range_2g, BMA250_update_time_64ms);
              delay(50);
              if (result == 0) {
                serialDebug("Addr " + String(accel_sensor.I2Caddress, HEX));
                sendJSON["Result"] = "OK";
                serializeJson(sendJSON, Serial);
                Serial.println();
                break;
              } else {
                sendJSON.clear();
                sendJSON["i2c0x18"] = "Fail";
                sendJSON["Error"] = "Init timeout";
                serializeJson(sendJSON, Serial);
                Serial.println();
              }
              delay(50);
            }

            // 4. Lectura y envío de datos si la inicialización fue exitosa
            if (result == 0) {
              for (int j = 0; j < step; j++) {
                accel_sensor.read();
                x = accel_sensor.X;
                y = accel_sensor.Y;
                z = accel_sensor.Z;

                if (x != -1 && y != -1 && z != -1) {
                  sendJSON.clear();
                  sendJSON["i2c0x18"] = "OK";
                  sendJSON["addr"] = String(accel_sensor.I2Caddress, HEX);
                  showJSON();
                  serializeJson(sendJSON, Serial);
                  Serial.println();
                  delay(100);  // Controla la tasa de muestreo hacia la web
                }
              }
            }
            break;
          }

        // ---------------------------------------------------------
        // CASO 3: Lectura de sensor vía I2C (Dirección 0x19)
        // ---------------------------------------------------------
        case 3:
          {
            sendJSON.clear();
            serialDebug("Test I2C Addr 0x19 Initialized...");

            // 1. Preparación de Buses
            SPI.end();
            pinMode(SDO_PIN, OUTPUT);

            // 2. Configuración de Hardware: Modo I2C y Dirección 0x19
            digitalWrite(PS_PIN, HIGH);
            digitalWrite(SDO_PIN, HIGH);
            delay(100);

            Wire.begin(SDA_PIN, SCL_PIN);
            int result = -1, step = 15;

            // 3. Intento de inicialización
            for (int i = 0; i < 3; i++) {
              result = accel_sensor.begin(BMA250_range_2g, BMA250_update_time_64ms);
              delay(50);
              if (result == 0) {
                serialDebug("Addr " + String(accel_sensor.I2Caddress, HEX));
                sendJSON["Result"] = "OK";
                serializeJson(sendJSON, Serial);
                Serial.println();
                break;
              } else {
                sendJSON.clear();
                sendJSON["i2c0x19"] = "Fail";
                sendJSON["Error"] = "Init timeout";
                serializeJson(sendJSON, Serial);
                Serial.println();
              }
              delay(100);
            }

            // 4. Lectura de ráfaga
            if (result == 0) {
              for (int j = 0; j < step; j++) {
                accel_sensor.read();
                x = accel_sensor.X;
                y = accel_sensor.Y;
                z = accel_sensor.Z;

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
            break;
          }

        // ---------------------------------------------------------
        // CASO 4: Lectura de sensor vía SPI
        // ---------------------------------------------------------
        case 4:
          {
            sendJSON.clear();
            serialDebug("Test SPI Initialized...");

            // 1. Preparación de Buses: Cierra I2C y libera pin SDO
            Wire.end();
            pinMode(SDO_PIN, INPUT);  // SDO funciona ahora como MISO

            // 2. Configuración de Hardware: Modo SPI
            digitalWrite(PS_PIN, LOW);
            delay(100);

            SPI.begin(SCL_PIN, SDO_PIN, SDA_PIN, CS_PIN);
            int result = accel_sensor.beginSPI(BMA250_range_2g, BMA250_update_time_64ms, CS_PIN, &SPI);
            int step = 15;

            // 3. Lectura de ráfaga o notificación de fallo
            if (result == 0) {
              sendJSON["Result"] = "OK";
              serializeJson(sendJSON, Serial);
              Serial.println();

              for (int j = 0; j < step; j++) {
                accel_sensor.read();
                x = accel_sensor.X;
                y = accel_sensor.Y;
                z = accel_sensor.Z;
                temp = ((accel_sensor.rawTemp * 0.5) + 24.0);

                if (x != -1 && y != -1 && z != -1) {
                  sendJSON.clear();
                  sendJSON["SPI"] = "OK";
                  showJSON();
                  sendJSON["Temperature"] = temp;  // Agregado térmico exclusivo de SPI
                  serializeJson(sendJSON, Serial);
                  Serial.println();
                  delay(100);
                }
              }
            } else {
              sendJSON.clear();
              sendJSON["SPI"] = "Fail";
              sendJSON["Error"] = "Init timeout";
              serializeJson(sendJSON, Serial);
              Serial.println();
            }
            break;
          }
      }
    }
  }
}