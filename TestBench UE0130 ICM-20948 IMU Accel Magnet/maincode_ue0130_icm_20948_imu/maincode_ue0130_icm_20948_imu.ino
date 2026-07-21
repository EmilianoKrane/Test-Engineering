/*
Firmware de prueba ICM20948


*/

#include <DevLab_ICM20948.h>
#include <Wire.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

/// ==== DECLARACIÓN DE PINES ====
#define RUN_BUTTON 4  // >> Botonera de Arranque - Pin para botón de inicio físico
#define CS_PIN 18     // Chip Select CS
#define SCK_PIN 6     // SPI SCK SCL
#define MOSI_PIN 7    // SPI MOSI SDA o SDI
#define MISO_PIN 2    // SPI MISO ADO o SDO

// ==== CREACIÓN DE OBJETOS ====
DevLab_ICM20948 imu;                   // Objeto del sensor inercial
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos
StaticJsonDocument<1024> sendJSON;     ///< Documento JSON para armar respuestas

#define ICM_ADDR 0x69  // 0x68 (AD0=LOW) or 0x69 Default (AD0=HIGH)

// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  StaticJsonDocument<255> doc;
  doc["debug"] = str;
  serializeJson(doc, Serial);
  Serial.println();
}


void setup() {
  Serial.begin(115200);
  delay(100);
  serialDebug("Serial Initialized...");

  // ==== DECLARACIÓN DE GPIOS ====
  pinMode(RUN_BUTTON, INPUT_PULLUP);
}

void loop() {

  // ==== Manejo del botón de arranque ====
  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();  // Limpia cualquier dato previo
    delay(100);        // Debounce

    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Arranque por botonera");
      delay(20);
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, Serial);  // Envío de datos por JSON a la PagWeb
      Serial.println();
    }
  }

  if (Serial.available()) {

    String inputJSON = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inputJSON);

    String Function = receiveJSON["Function"];


    int opc = 0;
    if (Function == "ping") opc = 1;             // {"Function":"ping"}
    else if (Function == "whoIam") opc = 2;      // {"Function":"whoIam"}
    else if (Function == "initSPI") opc = 3;     // {"Function":"initSPI"}
    else if (Function == "readSensor") opc = 4;  // {"Function":"readSensor"}


    switch (opc) {
      case 1:
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      case 2:
        {
          sendJSON.clear();
          Wire.begin(MOSI_PIN, SCK_PIN);
          bool whoIam = false;

          // ==== Inicialización del IMU I2C ====
          if (imu.beginI2C(ICM_ADDR, Wire, 400000)) {
            sendJSON["status"] = "initialized";

            // ==== Identificador único ====
            uint8_t who;
            if (imu.readWhoAmI(who)) {
              String whoIam = "0x" + String(who, HEX);
              sendJSON["whoIam"] = whoIam;
              whoIam = true;
            } else {
              sendJSON["whoIam"] = "FAIL";
            }

            if (whoIam) {
              if (!imu.setSensors(true, true, true)) {
                sendJSON["sensor"] = "not ready";
              }
              if (!imu.initMag()) {
                sendJSON["magnetometer"] = "not ready";
              }
            }
          } else {
            sendJSON["Result"] = "FAIL";
            sendJSON["error"] = "the sensor could not be initialized...";
          }

          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      case 3:
        {
          sendJSON.clear();
          SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
          if (!imu.beginSPI(CS_PIN, SPI, 1000000)) {
            Serial.println(F("ERROR: beginSPI() failed"));
          }

          if (!imu.setSensors(true, true, true)) {
            Serial.println(F("ERROR: setSensors failed"));
          }

          break;
        }

      case 4:
        {
          sendJSON.clear();
          float ax, ay, az;
          float gx, gy, gz;
          float tC;

          /** Read Accelerometer */
          if (imu.readAccel(ax, ay, az)) {
            Serial.print(F("ACC [g]: "));
            Serial.print(ax, 3);
            Serial.print(", ");
            Serial.print(ay, 3);
            Serial.print(", ");
            Serial.println(az, 3);
          } else {
            Serial.println(F("ACC read failed"));
          }

          /** Read Gyroscope */
          if (imu.readGyro(gx, gy, gz)) {
            Serial.print(F("GYR [dps]: "));
            Serial.print(gx, 2);
            Serial.print(", ");
            Serial.print(gy, 2);
            Serial.print(", ");
            Serial.println(gz, 2);
          } else {
            Serial.println(F("GYR read failed"));
          }

          /** Read Temperature */
          if (imu.readTemperature(tC)) {
            Serial.print(F("TMP [C]: "));
            Serial.println(tC, 2);
          } else {
            Serial.println(F("TMP read failed"));
          }



          break;
        }


      default:
        serialDebug("invalid option...");
        break;
    }
  }
}





/*

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


*/