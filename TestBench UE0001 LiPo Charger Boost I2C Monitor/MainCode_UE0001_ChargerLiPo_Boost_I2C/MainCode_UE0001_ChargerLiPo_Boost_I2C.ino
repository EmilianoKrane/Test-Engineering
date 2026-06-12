/*

*/

// ====   BIBLIOTECAS ====
#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// ==== DECLARACIÓN DE GPIOS ==== +
#define RUN_BUTTON 4  // >> GPIO04 Arranque por Botonera en TestBench
#define SDA_PIN 6
#define SCL_PIN 7

// ==== DECLARACIÓN DE OBJETOS ====
String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;                ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas

// ==== DECLARACIÓN DE REGISTRO Y VARIABLES GLOBALES ====
#define MAX17048_ADDR 0x36
#define REG_VCELL 0x02
#define REG_SOC 0x04
#define REG_MODE 0x06  // Registro para comandos especiales

// ==== UTILIDADES ====
void serialDebug(String str) {
  StaticJsonDocument<200> debugDoc;
  debugDoc["debug"] = str;
  serializeJson(debugDoc, Serial);
  Serial.println();
}

// ---- Función para leer registro de MAX17048 ----
uint16_t readRegister16(uint8_t reg) {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);  // Restart para mantener el bus

  Wire.requestFrom(MAX17048_ADDR, 2);

  if (Wire.available() == 2) {
    uint16_t msb = Wire.read();
    uint16_t lsb = Wire.read();
    return (msb << 8) | lsb;
  }
  return 0;
}

float readVoltage() {
  uint16_t rawVcell = readRegister16(REG_VCELL);
  return rawVcell * 0.000078125;
}

float readSOC() {
  uint16_t rawSoc = readRegister16(REG_SOC);
  return rawSoc / 256.0;
}

void sendQuickStart() {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(REG_MODE);
  Wire.write(0x40);  // MSB del comando 0x4000
  Wire.write(0x00);  // LSB del comando 0x4000
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);
  delay(100);
  serialDebug("Serial initialized...");

  Wire.begin(SDA_PIN, SCL_PIN);

  // ---- Configuración de GPIOS ----
}

void loop() {

  if (Serial.available()) {
    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "ping") opc = 1;             // {"Function": "ping"}
      else if (Function == "monitorMax") opc = 2;  // {"Function": "monitorMax"}

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
            sendQuickStart();

            float voltage = readVoltage();
            float soc = readSOC();

            if (soc > 100) sendJSON["error"] = "Floating VBAT Terminal...";
            else {
              sendJSON["Result"] = "OK";
              sendJSON["voltage"] = voltage;
              sendJSON["SOC"] = soc;
            }

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  }
}
