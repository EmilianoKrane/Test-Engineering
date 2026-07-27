/**
Este firmware funciona para 

*/


// ==== LIBRERIAS ====
#include <Wire.h>
#include <DevLab_BMI323.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// ==== DECLARACION DE GPIOS ====
#define SDA_PIN 6  // >> GPIO06 Señal de Datos SDA en bus I2C
#define SDA_PIN 7  // >> GPIO07 Señal de Reloj SCL en bus I2C

// ==== CREACIÓN DE OBJETOS =====
StaticJsonDocument<1024> receiveJSON;
StaticJsonDocument<1024> sendJSON;


// ==== DECLARACIÓN DE VARIABLES GLOBALES ====


// ==== FUNCIONE DE UTILIDAD ====
void serialDebug(String str) {
  StaticJsonDocument<255> doc;
  doc["debug"] = str;
  serializeJson(doc, Serial);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(100);
  serialDebug("Test BMI323 DevLab Module Initialized...");

  // ==== Declaración de Pines ====
}

void loop() {

  if (Serial.available()) {

    String inJson = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inJson);

    if (error) {
      serialDebug(String("Error JSON: ") + error.c_str());
    } else {

      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "ping") opc = 1;  // {"Function":"ping"}
      else if (Function == "initI2C") opc = 2; // {"Function":"initI2C"}


      switch (opc) {
        case 1:
          {
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }


        default: serialDebug("error invalid option."); break;
      }
    }
  }
}
