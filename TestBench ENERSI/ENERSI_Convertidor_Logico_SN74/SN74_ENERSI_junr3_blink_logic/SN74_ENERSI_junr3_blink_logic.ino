/*
Firmware SN74 ENERSI. La JUNR3 se encarga de entregar dos salidas digitales oscilando entre 
0 - 5V (B1 y B3). De igual forma, realiza lecturas analógicas en los pines B2 y B4. 
*/

#include "Arduino.h"
#include <ArduinoJson.h>
#include <HardwareSerial.h>

#define B1_PIN 14  // A0
#define B2_PIN 15
#define B3_PIN 16
#define B4_PIN 17

// ==== DECLARACIÓN DE OBJETO JSON ====
StaticJsonDocument<128> receiveJSON;
StaticJsonDocument<128> sendJSON;

const float ADC_RESOLUTION = 1024.0;
const float V_REF = 5.0;

// ==== FUNCIONES DE UTILIDAD ====
// Función auxiliar para generar el JSON basado en el voltaje
void getStatusJSON(String pinName, float voltage) {

  String status = (voltage >= 1.2 && voltage <= 1.7) ? "OK" : "ERROR";

  StaticJsonDocument<96> doc;
  doc.clear();
  doc["device"] = "JUNR3";
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
  sendJSON["Module"] = "JUNR3 logic converter ENERSI";
  serializeJson(sendJSON, Serial);
  Serial.println();

  // ==== DECLARACIÓN DE ENTRADAS Y SALIDAS ====
  pinMode(B1_PIN, OUTPUT);
  pinMode(B2_PIN, INPUT);
  pinMode(B3_PIN, OUTPUT);
  pinMode(B4_PIN, INPUT);
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
            sendJSON["ping"] = "pong_JUNR3";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();
            int delay_ms = 500;
            for (int i = 0; i < 5; i++) {
              digitalWrite(B1_PIN, HIGH);
              digitalWrite(B3_PIN, HIGH);
              delay(delay_ms);
              digitalWrite(B1_PIN, LOW);
              digitalWrite(B3_PIN, LOW);
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
            int delay_ms = 1000;
            for (int i = 0; i < 5; i++) {

              int raw_b2 = analogRead(B2_PIN);
              int raw_b4 = analogRead(B4_PIN);

              float voltage_b2 = (raw_b2 / ADC_RESOLUTION) * V_REF;
              float voltage_b4 = (raw_b4 / ADC_RESOLUTION) * V_REF;
              getStatusJSON("B2", voltage_b2);
              getStatusJSON("B4", voltage_b4);
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
