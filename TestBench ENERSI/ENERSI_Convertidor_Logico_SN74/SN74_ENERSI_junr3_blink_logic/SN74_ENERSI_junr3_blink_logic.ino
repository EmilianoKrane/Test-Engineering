/*
Firmware SN74 ENERSI. La JUNR3 se encarga de entregar dos salidas digitales oscilando entre 
0 - 5V (B1 y B3). De igual forma, realiza lecturas analógicas en los pines B2 y B4. 
*/

#include "Arduino.h"
#include <ArduinoJson.h>

#define B1_PIN 14  // A0
#define B2_PIN 15  // A1
#define B3_PIN 16  // A2
#define B4_PIN 17  // A3

StaticJsonDocument<128> receiveJSON;
StaticJsonDocument<128> sendJSON;

const float ADC_RESOLUTION = 1024.0;
const float V_REF = 5.0;

// ==== NUEVA FUNCIÓN DE JSON ====
// Ahora recibe el estado evaluado por los flancos y el conteo de pulsos
void getStatusJSON(String pinName, String status, int pulses) {
  StaticJsonDocument<96> doc;
  doc.clear();
  doc["device"] = "JUNR3";
  doc["pin"] = pinName;
  doc["pulses"] = pulses;
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

  pinMode(B1_PIN, OUTPUT);
  pinMode(B2_PIN, INPUT);
  pinMode(B3_PIN, OUTPUT);
  pinMode(B4_PIN, INPUT);
}

void loop() {
  if (Serial.available()) {
    String inputJSON = Serial.readStringUntil('\n');
    inputJSON.trim();
    DeserializationError error = deserializeJson(receiveJSON, inputJSON);

    if (error) {
      sendJSON.clear();
      sendJSON["status"] = "FAIL";
      sendJSON["error"] = error.c_str();
      serializeJson(sendJSON, Serial);
      Serial.println();
    } else {
      String Function = receiveJSON["Function"];
      int opc = 0;
      if (Function == "ping") opc = 1;
      else if (Function == "blink") opc = 2;
      else if (Function == "analogRead") opc = 3;

      switch (opc) {
        case 1: {
            sendJSON.clear();
            sendJSON["ping"] = "pong_JUNR3";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
        }
        case 2: {
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
        case 3: {
            sendJSON.clear();
            int pulses_b2 = 0;
            int pulses_b4 = 0;
            bool state_b2 = false; // false = LOW, true = HIGH
            bool state_b4 = false;

            unsigned long startTime = millis();
            
            // Muestreo continuo por 6 segundos
            while (millis() - startTime < 6000) {
              float v_b2 = (analogRead(B2_PIN) / ADC_RESOLUTION) * V_REF;
              float v_b4 = (analogRead(B4_PIN) / ADC_RESOLUTION) * V_REF;

              // Detección de flanco B2 (Umbral alto > 2.5V, Umbral bajo < 1.0V)
              if (!state_b2 && v_b2 > 2.5) state_b2 = true;
              else if (state_b2 && v_b2 < 1.0) { state_b2 = false; pulses_b2++; }

              // Detección de flanco B4
              if (!state_b4 && v_b4 > 2.5) state_b4 = true;
              else if (state_b4 && v_b4 < 1.0) { state_b4 = false; pulses_b4++; }

              delay(10); // Muestreo cada 10ms
            }

            // Exigimos al menos 3 pulsos completos detectados para considerarlo OK
            String status_b2 = (pulses_b2 >= 3) ? "OK" : "ERROR";
            String status_b4 = (pulses_b4 >= 3) ? "OK" : "ERROR";

            getStatusJSON("B2", status_b2, pulses_b2);
            getStatusJSON("B4", status_b4, pulses_b4);

            sendJSON.clear();
            sendJSON["status"] = "finished reading";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
        }
      }
    }
  }
}