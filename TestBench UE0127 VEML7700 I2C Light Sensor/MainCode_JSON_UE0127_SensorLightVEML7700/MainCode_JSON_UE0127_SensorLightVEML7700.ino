/*
Este firmware es el codigo principal para usar en el testbench como prueba del modulo UE0127 sensor light veml7700
Se comunica por i2c la pulsarc6 del testbenh con el sensor de luz


*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include "Adafruit_VEML7700.h"

// ==== DECLARACIÓN DE GPIOS ====
#define SDA_PIN 6   // -> GPIO06 SDA Comunicación I2C con el sensor
#define SCL_PIN 7   // -> GPIO07 SCL Comunicación I2C con el sensor
#define RX2_PIN 15  // -> GPIO15 RX UART2 TestBench
#define TX2_PIN 19  // -> GPIO19 TX UART2 TestBench


// ==== CREACIÓN DE OBJETOS ====
HardwareSerial PagWeb(1);
Adafruit_VEML7700 veml = Adafruit_VEML7700();

String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;                ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas


void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  Serial.println("{\"debug\": \"" + str + "\"}");
}

void pagwebDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  PagWeb.println("{\"debug\": \"" + str + "\"}");
}

void setup() {

  // ==== Inicialización de Comunicación Serial ====
  Serial.begin(115200);
  PagWeb.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  delay(100);
  serialDebug("Serial Initialized...");
  pagwebDebug("Test Initialized...");

  // ==== Inicialización bus I2C ====
  Wire.begin(SDA_PIN, SCL_PIN);
}

void loop() {

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');                              // JSON crudo se lee hasta salto de linea
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Se asigna el String al objeto JSON

    // ==== Claves recibidas por el JSON ====
    String Function = receiveJSON["Function"];

    int opc = 0;
    if (Function == "ping") opc = 1;             // {"Function":"ping"}
    else if (Function == "scanDis") opc = 2;     // {"Function":"scanDis"}
    else if (Function == "initSensor") opc = 3;  // {"Function":"initSensor"}

    switch (opc) {
      case 1:  // -> Respuesta UART puente frontend <-> testbench
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 2:  // -> Escáner de Dispositivos I2C
        {
          sendJSON.clear();
          for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
              String addrHex = "";
              if (addr < 16) addrHex = "0";
              addrHex = addrHex + String(addr, HEX);
              serialDebug("I2C device found at 0x" + addrHex);
              pagwebDebug("I2C device found at 0x" + addrHex);
            }
          }
          break;
        }

      case 3:
        {
          sendJSON.clear();

          
          break;
        }



      default: break;
    }
  }
}
