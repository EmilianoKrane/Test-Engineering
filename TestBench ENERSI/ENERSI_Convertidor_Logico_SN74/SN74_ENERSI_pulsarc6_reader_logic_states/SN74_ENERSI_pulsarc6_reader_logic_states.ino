/*
Firmware SN74 ENERSI. La pulsar se encarga de entregar dos salidas digitales oscilando 
entre 0V y 3.3V (A2 y A4), y puede realizar lecturas analogicas en A1 y A3
*/

#include "Arduino.h"
#include <ArduinoJson.h>
#include <HardwareSerial.h>

#define A1_PIN 0
#define A2_PIN 1
#define A3_PIN 2
#define A4_PIN 3

// ==== DECLARACIÓN DE OBJETO JSON ====
StaticJsonDocument<1024> receiveJSON;
StaticJsonDocument<1024> sendJSON;

// ==== FUNCIONES DE UTILIDAD ====
// Función auxiliar para generar el JSON basado en el voltaje
void getStatusJSON(String pinName, float voltage) {

  String status = (voltage >= 1.2 && voltage <= 1.7) ? "OK" : "ERROR";

  StaticJsonDocument<256> doc;
  doc.clear();
  doc["device"] = "PULSARC6";
  doc["pin"] = pinName;
  doc["voltage"] = String(voltage, 2);
  doc["status"] = status;
  serializeJson(doc, Serial);
  Serial.println();
}



void setup() {
  Serial.begin(115200);
  delay(1000);

  sendJSON.clear();
  sendJSON["System"] = "Ready";
  sendJSON["Module"] = "PULSAR C6 logic converter ENERSI";
  serializeJson(sendJSON, Serial);
  Serial.println();

  // ==== DECLARACIÓN DE ENTRADAS Y SALIDAS ====
  pinMode(A1_PIN, INPUT);
  pinMode(A2_PIN, OUTPUT);
  pinMode(A3_PIN, INPUT);
  pinMode(A4_PIN, OUTPUT);
}


void loop() {

  if (Serial.available()) {

    String inputJSON = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inputJSON);

    if (error) {
      sendJSON.clear();
      sendJSON["status"] = "FAIL";
      sendJSON["error"] = String("Invalid JSON: ") + error.c_str();
      serializeJson(sendJSON, Serial);
      Serial.println();
    } else {

      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "ping") opc = 1;             // {"Function":"ping"}
      else if (Function == "blink") opc = 2;       // {"Function":"blink"}
      else if (Function == "analogRead") opc = 3;  // {"Function":"analogRead"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong_C6";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();
            int delay_ms = 1000;
            for (int i = 0; i < 5; i++) {
              digitalWrite(A2_PIN, HIGH);
              digitalWrite(A4_PIN, HIGH);
              delay(delay_ms);
              digitalWrite(A2_PIN, LOW);
              digitalWrite(A4_PIN, LOW);
              delay(delay_ms);
            }

            sendJSON["status"] = "finished blink";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:
          {
            sendJSON.clear();
            int delay_ms = 500;
            for (int i = 0; i < 10; i++) {
              float voltage_a1 = analogReadMilliVolts(A1_PIN) / 1000.0;
              float voltage_a3 = analogReadMilliVolts(A3_PIN) / 1000.0;
              getStatusJSON("A1", voltage_a1);
              getStatusJSON("A3", voltage_a3);
              delay(delay_ms);
            }

            sendJSON["status"] = "finished reading";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  }
}
