/*
Firmware de prueba ICM20948 - Seguridad de Buses Mejorada y Timeouts
*/

#include <DevLab_ICM20948.h>
#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

// ==== DECLARACIÓN DE PINES ====
#define RUN_BUTTON 6  // >> Botonera de Arranque
#define CS_PIN 18     // Chip Select CS
#define SCK_PIN 22     // SPI SCK  / I2C SCL
#define MOSI_PIN 23    // SPI MOSI / I2C SDA
#define MISO_PIN 2    // SPI MISO SDO ADO

// ==== CREACIÓN DE OBJETOS ====
DevLab_ICM20948 imu;
StaticJsonDocument<1024> receiveJSON;
StaticJsonDocument<1024> sendJSON;

#define ICM_ADDR 0x69  // 0x68 (AD0=LOW) or 0x69 Default (AD0=HIGH)

// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  StaticJsonDocument<255> doc;
  doc["debug"] = str;
  serializeJson(doc, Serial);
  Serial.println();
}

// ==== CANDADO DE HARDWARE: APAGADO DE BUSES ====
// Detiene los protocolos y pasa los pines a alta impedancia.
// Previene cuelgues del bus y alimentación parásita al intercambiar placas.
void releaseBuses() {
  Wire.end();
  SPI.end();

  pinMode(SCK_PIN, INPUT);
  pinMode(MOSI_PIN, INPUT);
  pinMode(MISO_PIN, INPUT);
  pinMode(CS_PIN, INPUT);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  serialDebug("Serial Initialized...");
  pinMode(RUN_BUTTON, INPUT_PULLUP);
}

void loop() {
  // ==== Manejo del botón de arranque ====
  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();
    delay(100);
    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON["Run"] = "OK";
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
  }

  if (Serial.available()) {
    String inputJSON = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inputJSON);

    if (error) {
      serialDebug("Invalid JSON syntax");
      return;
    }

    String Function = receiveJSON["Function"];

    int opc = 0;
    if (Function == "ping") opc = 1;             // {"Function":"ping"}
    else if (Function == "whoIam") opc = 2;      // {"Function":"whoIam"}
    else if (Function == "initSPI") opc = 3;     // {"Function":"initSPI"}
    else if (Function == "readSensor") opc = 4;  // {"Function":"readSensor"}
    else if (Function == "release") opc = 5;     // {"Function": "release"}


    switch (opc) {
      case 1:  // PING
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      case 2:  // TEST I2C (whoIam)
        {
          sendJSON.clear();

          // Asegurar que SPI no esté secuestrando los pines
          releaseBuses();
          Wire.begin(MOSI_PIN, SCK_PIN);
          Wire.setTimeOut(150);  // 150ms timeout
          bool isWhoAmIOk = false;

          // ==== Inicialización del IMU I2C ====
          if (imu.beginI2C(ICM_ADDR, Wire, 400000)) {
            sendJSON["status"] = "initialized";

            // ==== Identificador único ====
            uint8_t who;
            if (imu.readWhoAmI(who)) {
              String hexID = "0x" + String(who, HEX);
              sendJSON["whoIam"] = hexID;
              sendJSON["Result"] = "OK";
              isWhoAmIOk = true;
            } else {
              sendJSON["whoIam"] = "FAIL";
              releaseBuses();  
            }

            if (isWhoAmIOk) {
              if (!imu.setSensors(true, true, true)) {
                sendJSON["sensor"] = "not ready";
                releaseBuses();  // LIMPIEZA: Falló configuración de sensores
              } else if (!imu.initMag()) {
                sendJSON["magnetometer"] = "not ready";
                releaseBuses();  // LIMPIEZA: Falló configuración de magnetómetro
              }
            }
          } else {
            sendJSON["status"] = "FAIL";
            sendJSON["error"] = "Sensor could not be initialized via I2C";
            releaseBuses();  // LIMPIEZA: El sensor no respondió al inicio
          }

          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      case 3:  // INIT SPI
        {
          sendJSON.clear();

          // Apagar I2C antes de iniciar SPI en los mismos pines
          releaseBuses();
          SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

          // SPI no se "cuelga" como I2C, pero sí devuelve error si no hay conexión
          if (!imu.beginSPI(CS_PIN, SPI, 1000000)) {
            sendJSON["status"] = "FAIL";
            sendJSON["error"] = "beginSPI() failed";
            releaseBuses();  // LIMPIEZA: Sensor no detectado por SPI
          } else if (!imu.setSensors(true, true, true)) {
            sendJSON["status"] = "FAIL";
            sendJSON["error"] = "setSensors() failed via SPI";
            releaseBuses();  // LIMPIEZA: Falló configuración
          } else {
            sendJSON["status"] = "OK";
            sendJSON["Result"] = "OK";
            sendJSON["msg"] = "SPI Initialized";
          }

          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      case 4:  // READ SENSOR (SPI)
        {
          for (int i = 0; i < 40; i++) {
            sendJSON.clear();
            float ax, ay, az, gx, gy, gz, tC;
            bool readSuccess = true;

            JsonObject acc = sendJSON.createNestedObject("ACC");
            if (imu.readAccel(ax, ay, az)) {
              acc["x"] = ax;
              acc["y"] = ay;
              acc["z"] = az;
            } else {
              readSuccess = false;
            }

            JsonObject gyr = sendJSON.createNestedObject("GYR");
            if (imu.readGyro(gx, gy, gz)) {
              gyr["x"] = gx;
              gyr["y"] = gy;
              gyr["z"] = gz;
            } else {
              readSuccess = false;
            }

            if (imu.readTemperature(tC)) {
              sendJSON["TMP"] = tC;
            } else {
              readSuccess = false;
            }

            sendJSON["status"] = readSuccess ? "OK" : "READ_FAIL";

            serializeJson(sendJSON, Serial);
            Serial.println();
            delay(100);
          }
          break;
        }

      case 5:  // RELEASE BUSES (Cierre de prueba)
        {
          releaseBuses();
          sendJSON.clear();
          sendJSON["status"] = "RELEASED";
          sendJSON["msg"] = "Buses in High-Z state. Safe to remove PCB.";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      default:
        serialDebug("invalid option...");
        break;
    }
  }
}