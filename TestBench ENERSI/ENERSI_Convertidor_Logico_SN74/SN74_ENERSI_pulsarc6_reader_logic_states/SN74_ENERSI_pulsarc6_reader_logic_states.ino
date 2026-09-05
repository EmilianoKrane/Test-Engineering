/*
Firmware SN74 ENERSI. La pulsar se encarga de entregar dos salidas digitales oscilando 
entre 0V y 3.3V (A2 y A4), y puede realizar lecturas analogicas en A1 y A3
*/

#include "Arduino.h"
#include <ArduinoJson.h>

#define A1_PIN 0
#define A2_PIN 1
#define A3_PIN 2
#define A4_PIN 3

StaticJsonDocument<512> receiveJSON;
StaticJsonDocument<512> sendJSON;

// ==== NUEVA FUNCIÓN DE JSON ====
void getStatusJSON(String pinName, String status, int pulses) {
  StaticJsonDocument<128> doc;
  doc.clear();
  doc["device"] = "PULSARC6";
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
  sendJSON["Module"] = "PULSAR C6 logic converter ENERSI";
  serializeJson(sendJSON, Serial);
  Serial.println();

  pinMode(A1_PIN, INPUT);
  pinMode(A2_PIN, OUTPUT);
  pinMode(A3_PIN, INPUT);
  pinMode(A4_PIN, OUTPUT);
}

void loop() {
  if (Serial.available()) {
    String inputJSON = Serial.readStringUntil('\n');
    inputJSON.trim();
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
      if (Function == "ping") opc = 1;
      else if (Function == "blink") opc = 2;
      else if (Function == "analogRead") opc = 3;

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
            int delay_ms = 500;
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
            int pulses_a1 = 0;
            int pulses_a3 = 0;
            bool state_a1 = false;
            bool state_a3 = false;

            unsigned long startTime = millis();

            // Muestreo continuo por 6 segundos
            while (millis() - startTime < 6000) {
              float v_a1 = analogReadMilliVolts(A1_PIN) / 1000.0;
              float v_a3 = analogReadMilliVolts(A3_PIN) / 1000.0;

              // Detección A1: divisor de tensión (Alto 1.2V a 1.7V, Bajo < 0.5V)
              if (!state_a1 && v_a1 >= 2.2 && v_a1 <= 3.5) state_a1 = true;
              else if (state_a1 && v_a1 < 0.5) {
                state_a1 = false;
                pulses_a1++;
              }

              // Detección A3: divisor de tensión (Alto 1.2V a 1.7V, Bajo < 0.5V)
              if (!state_a3 && v_a3 >= 2.2 && v_a3 <= 3.5) state_a3 = true;
              else if (state_a3 && v_a3 < 0.5) {
                state_a3 = false;
                pulses_a3++;
              }

              delay(10);  // Muestreo cada 10ms
            }

            // Exigimos al menos 3 pulsos completos detectados para considerarlo OK
            String status_a1 = (pulses_a1 >= 3) ? "OK" : "ERROR";
            String status_a3 = (pulses_a3 >= 3) ? "OK" : "ERROR";

            getStatusJSON("A1", status_a1, pulses_a1);
            getStatusJSON("A3", status_a3, pulses_a3);

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