/**
Este firmware funciona para 

*/

// ==== LIBRERIAS ====
#include <SPI.h>
#include <Wire.h>
#include <DevLab_BMI323.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// ==== DECLARACION DE GPIOS ====
#define RUN_BUTTON 4
#define CS_PIN 18   // Chip Select CS
#define SCK_PIN 6   // SPI SCK  / I2C SCL
#define MOSI_PIN 7  // SPI MOSI / I2C SDAs
#define MISO_PIN 2  // SPI MISO SDO ADO SAO

#define SPI_FAST_SPEED 10000000  // 10MHz

// ==== CREACIÓN DE OBJETOS =====
StaticJsonDocument<1024> receiveJSON;
StaticJsonDocument<1024> sendJSON;

DevLab_BMI323 imu0x68(Wire, 0x68);
DevLab_BMI323 imu0x69(Wire, 0x69);
DevLab_BMI323 imuSpi(SPI, CS_PIN, MISO_PIN, MOSI_PIN, SCK_PIN, SPI_FAST_SPEED);

BMI323_SensorData data;

// ==== DECLARACIÓN DE VARIABLES GLOBALES ====
uint8_t activeAddress = 0x69;  // Dirección activa por defecto

// ==== FUNCIONE DE UTILIDAD ====
void serialDebug(String str) {
  StaticJsonDocument<255> doc;
  doc["debug"] = str;
  serializeJson(doc, Serial);
  Serial.println();
}

void releaseBuses() {
  Wire.end();
  SPI.end();

  // 1. Bloquear el CS en HIGH DE INMEDIATO
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  delay(10);  // Darle tiempo al sensor a salir del modo SPI

  // 2. Liberar el resto de los pines
  pinMode(SCK_PIN, INPUT);
  pinMode(MOSI_PIN, INPUT);
  pinMode(MISO_PIN, INPUT);
}
void setup() {
  Serial.begin(115200);
  delay(100);
  serialDebug("Test BMI323 DevLab Module Initialized...");

  // ==== Declaración de Pines ====
  pinMode(RUN_BUTTON, INPUT_PULLUP);
}

void loop() {

  // ==== Manejo del botón de arranque ====
  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();  // Limpia cualquier dato previo
    delay(100);        // Debounce

    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Arranque por botonera");
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, Serial);  // Envío de datos por JSON a la PagWeb
      Serial.println();
    }
  }

  if (Serial.available()) {

    String inJson = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inJson);

    if (error) {
      serialDebug(String("error JSON: ") + error.c_str());
    } else {

      String Function = receiveJSON["Function"];
      String Address = receiveJSON["Address"] | "0x69";

      int opc = 0;
      if (Function == "ping") opc = 1;             // {"Function":"ping"}
      else if (Function == "initI2C") opc = 2;     // {"Function":"initI2C", "Address":"0x68"}
      else if (Function == "initSPI") opc = 3;     // {"Function":"initSPI"}
      else if (Function == "readSensor") opc = 4;  // {"Function":"readSensor"}
      else if (Function == "readSPI") opc = 5;     // {"Function":"readSPI"}

      switch (opc) {
        case 1:
          {
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();

            activeAddress = 0x69;  // Valor por defecto en caso de no recibir parámetro
            bool isbusI2Cok = false;
            //releaseBuses();
            // Wire.begin(MOSI_PIN, SCK_PIN);
            // Wire.setTimeOut(1000);  // 150ms timeout

            // ==== Parseo de dirección seleccionada por el usuario ====
            if (receiveJSON.containsKey("Address")) {
              const char* addrStr = receiveJSON["Address"];
              activeAddress = (uint8_t)strtol(addrStr, NULL, 16);
            }

            pinMode(MISO_PIN, OUTPUT);
            if (activeAddress == 0x68) {
              digitalWrite(MISO_PIN, LOW);  // MISO (SDO) a GND = 0x68
              delay(200);
              isbusI2Cok = imu0x68.begin(MOSI_PIN, SCK_PIN, 400000);
              Serial.println("Sensor en 0x68 inicializado");
            } else {
              digitalWrite(MISO_PIN, HIGH);  // MISO (SDO) a VDD = 0x69
              delay(200);
              isbusI2Cok = imu0x69.begin(MOSI_PIN, SCK_PIN, 400000);
              Serial.println("Sensor en 0x69 inicializado");
            }

            if (!isbusI2Cok) {
              sendJSON["status"] = "FAIL";
              sendJSON["error"] = "BMI323 initialization failed.";
            } else {
              sendJSON["Result"] = "OK";
              sendJSON["status"] = "initialized";
            }

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:
          {
            sendJSON.clear();
            serialDebug("Test SPI Initialized...");
            SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, -1);
            if (!imuSpi.begin()) {
              Serial.println("Error : BMI323 initialization failed");
              break;
            }

            Serial.println("BMI323 Initialized succesfully");
            imuSpi.test_chip_id(BMI323_CHIP_ID, REG_CHIP_ID);

            break;
          }

        case 4:
          {
            sendJSON.clear();

            for (int i = 0; i < 10; i++) {
              bool readSuccess = false;

              // Leemos dinámicamente según la dirección que se inicializó
              if (activeAddress == 0x68) {
                readSuccess = imu0x68.readData(data);
                //serialDebug("Sensor inicializado en 0x68");
              } else if (activeAddress == 0x69) {
                readSuccess = imu0x69.readData(data);
                //serialDebug("Sensor inicializado en 0x69");
              }

              if (readSuccess) {
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
              // El delay debe ir fuera de la lectura exitosa para no saturar el bus si hay fallo
              delay(100);
            }

            break;
          }

        case 5:
          {
            sendJSON.clear();
            for (int i = 0; i < 15; i++) {
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

            break;
          }


        default: serialDebug("error invalid option."); break;
      }
    }
  }
}
