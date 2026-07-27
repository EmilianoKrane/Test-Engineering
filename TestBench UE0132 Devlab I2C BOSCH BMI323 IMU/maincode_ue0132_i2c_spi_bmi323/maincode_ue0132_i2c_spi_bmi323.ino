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
#define RUN_BUTTON 6  // >> Botonera de Arranque
#define CS_PIN 18     // Chip Select CS
#define SCK_PIN 23    // SPI SCK  / I2C SCL
#define MOSI_PIN 22   // SPI MOSI / I2C SDAs
#define MISO_PIN 2    // SPI MISO SDO ADO

// ==== CREACIÓN DE OBJETOS =====
StaticJsonDocument<1024> receiveJSON;
StaticJsonDocument<1024> sendJSON;

DevLab_BMI323 imu0x68(Wire, 0x68);
DevLab_BMI323 imu0x69(Wire, 0x69);

BMI323_SensorData data;

// ==== DECLARACIÓN DE VARIABLES GLOBALES ====


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

  pinMode(SCK_PIN, INPUT);
  pinMode(MOSI_PIN, INPUT);
  pinMode(MISO_PIN, INPUT);
  pinMode(CS_PIN, INPUT);
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

            // ==== Inicialización y limpieza de buses ====
            uint8_t Addr = 0x69;  // Declaramos el valor por defecto (105)
            bool isbusI2Cok = false;
            releaseBuses();
            Wire.begin(MOSI_PIN, SCK_PIN);
            Wire.setTimeOut(150);  // 150ms timeout

            // ==== Parseo de dirección seleccionada por el usuario ====
            if (receiveJSON.containsKey("Address")) {
              const char* addrStr = receiveJSON["Address"];
              Addr = (uint8_t)strtol(addrStr, NULL, 16);  // Conversión a hexadecimal
            }

            pinMode(MISO_PIN, OUTPUT);
            if (Addr == 0x68) {
              digitalWrite(MISO_PIN, LOW);
              delay(200);
              Serial.println(isbusI2Cok);
              isbusI2Cok = imu0x68.begin(MOSI_PIN, SCK_PIN, 400000);
              Serial.println(isbusI2Cok);
              Serial.println("Sensor en 0x68 inicializado");
            } else {
              digitalWrite(MISO_PIN, HIGH);
              delay(200);
              Serial.println(isbusI2Cok);
              isbusI2Cok = imu0x69.begin(MOSI_PIN, SCK_PIN, 400000);
              Serial.println(isbusI2Cok);
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


            break;
          }

        case 4:
          {
            sendJSON.clear();

            for (int i = 0; i < 10; i++) {

              if (imu0x68.readData(data)) {
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
                delay(100);
              } else {
                Serial.println("ERROR: Failed to read BMI323 data.");
              }
            }

            break;
          }



        default: serialDebug("error invalid option."); break;
      }
    }
  }
}
