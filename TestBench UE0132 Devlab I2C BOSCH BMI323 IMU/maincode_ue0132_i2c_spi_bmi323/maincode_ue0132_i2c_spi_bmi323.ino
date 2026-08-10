/**
Este firmware funciona para la validación del módulo BMI323
Integra prevención de bloqueos de bus y reportes estrictamente en JSON.

** Se realizó una modificación en la funcion de inicialización de begin para i2c 
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
int noValues = 20;             // Valor total de lecturas a realizar por protocolo

// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  StaticJsonDocument<255> doc;
  doc["debug"] = str;
  serializeJson(doc, Serial);
  Serial.println();
}

void releaseBuses() {
  Wire.end();
  SPI.end();

  // 1. Bloquear el CS en HIGH DE INMEDIATO para sacar al sensor de modo SPI
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  delay(10);

  // 2. Liberar el resto de los pines a un estado de alta impedancia
  pinMode(SCK_PIN, INPUT);
  pinMode(MOSI_PIN, INPUT);
  pinMode(MISO_PIN, INPUT);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  sendJSON["System"] = "Ready";
  sendJSON["Module"] = "BMI323 DevLab";
  serializeJson(sendJSON, Serial);
  Serial.println();

  // ==== Declaración de Pines ====
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

    String inJson = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inJson);

    if (error) {
      sendJSON.clear();
      sendJSON["status"] = "FAIL";
      sendJSON["error"] = String("Invalid JSON: ") + error.c_str();
      serializeJson(sendJSON, Serial);
      Serial.println();
    } else {

      String Function = receiveJSON["Function"];
      String Address = receiveJSON["Address"] | "0x69";

      int opc = 0;
      if (Function == "ping") opc = 1;           // {"Function":"ping"}
      else if (Function == "init_i2c") opc = 2;  // {"Function":"init_i2c", "Address":"0x68"}
      else if (Function == "init_spi") opc = 3;  // {"Function":"init_spi"}
      else if (Function == "read_i2c") opc = 4;  // {"Function":"read_i2c"}
      else if (Function == "read_spi") opc = 5;  // {"Function":"read_spi"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 2:  // Inicialización I2C
          {
            sendJSON.clear();
            activeAddress = 0x69;
            bool isbusI2Cok = false;

            releaseBuses();  // Limpia hardware cruzado

            // Protección vital contra bloqueos del bus I2C
            Wire.begin(MOSI_PIN, SCK_PIN);
            Wire.setTimeOut(150);

            // Parseo de dirección
            if (receiveJSON.containsKey("Address")) {
              const char* addrStr = receiveJSON["Address"];
              activeAddress = (uint8_t)strtol(addrStr, NULL, 16);
            }

            pinMode(MISO_PIN, OUTPUT);
            if (activeAddress == 0x68) {
              digitalWrite(MISO_PIN, LOW);  // MISO (SDO) a GND = 0x68
              delay(200);
              isbusI2Cok = imu0x68.begin(MOSI_PIN, SCK_PIN, 400000);
            } else {
              digitalWrite(MISO_PIN, HIGH);  // MISO (SDO) a VDD = 0x69
              delay(200);
              isbusI2Cok = imu0x69.begin(MOSI_PIN, SCK_PIN, 400000);
            }

            if (!isbusI2Cok) {
              sendJSON["status"] = "FAIL";
              sendJSON["error"] = "BMI323 I2C initialization failed.";
            } else {
              sendJSON["Result"] = "OK";
              sendJSON["status"] = "OK";
              sendJSON["message"] = "I2C Initialized";
              sendJSON["address"] = (activeAddress == 0x68) ? "0x68" : "0x69";
            }

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:  // Inicialización SPI
          {
            sendJSON.clear();
            releaseBuses();  // Aseguramos que los pines no tengan estados residuales del I2C

            SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, -1);
            if (!imuSpi.begin()) {
              sendJSON["status"] = "FAIL";
              sendJSON["error"] = "BMI323 SPI initialization failed.";
            } else {
              sendJSON["Result"] = "OK";
              sendJSON["status"] = "OK";
              sendJSON["message"] = "SPI Initialized";

              // Opcional: test_chip_id imprime directo a Serial en tu librería,
              // puedes comentarlo si requieres 100% limpieza JSON.
              // imuSpi.test_chip_id(BMI323_CHIP_ID, REG_CHIP_ID);
            }

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 4:  // Lectura I2C
          {
            for (int i = 0; i < noValues; i++) {
              sendJSON.clear();
              bool readSuccess = false;

              if (activeAddress == 0x68) {
                readSuccess = imu0x68.readData(data);
              } else if (activeAddress == 0x69) {
                readSuccess = imu0x69.readData(data);
              }

              if (readSuccess) {
                sendJSON["status"] = "OK";
                sendJSON["protocol"] = "I2C";
                sendJSON["accX"] = data.accX;
                sendJSON["accY"] = data.accY;
                sendJSON["accZ"] = data.accZ;
                sendJSON["gyrX"] = data.gyrX;
                sendJSON["gyrY"] = data.gyrY;
                sendJSON["gyrZ"] = data.gyrZ;
                sendJSON["tempC"] = data.temperatureC;
              } else {
                sendJSON["status"] = "FAIL";
                sendJSON["error"] = "I2C Read timeout/failure";
              }

              serializeJson(sendJSON, Serial);
              Serial.println();
              delay(100);
            }
            break;
          }

        case 5:  // Lectura SPI
          {
            for (int i = 0; i < noValues; i++) {
              sendJSON.clear();

              if (imuSpi.readData(data)) {
                sendJSON["status"] = "OK";
                sendJSON["protocol"] = "SPI";
                sendJSON["accX"] = data.accX;
                sendJSON["accY"] = data.accY;
                sendJSON["accZ"] = data.accZ;
                sendJSON["gyrX"] = data.gyrX;
                sendJSON["gyrY"] = data.gyrY;
                sendJSON["gyrZ"] = data.gyrZ;
                sendJSON["tempC"] = data.temperatureC;
              } else {
                sendJSON["status"] = "FAIL";
                sendJSON["error"] = "SPI Read failure";
              }

              serializeJson(sendJSON, Serial);
              Serial.println();
              delay(100);
            }
            break;
          }

        default:
          {
            sendJSON.clear();
            sendJSON["status"] = "FAIL";
            sendJSON["error"] = "Invalid function requested";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  }
}