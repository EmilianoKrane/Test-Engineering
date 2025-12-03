/* 
===== CÓDIGO TEMT6000 JSON Integración ====
*/

#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de pines
#define RX2 D4
#define TX2 D5
#define SENSOR_PIN 4

#define RELAYCom 20  // Relevador de puerto COM

// ==== Declaración de objetos
HardwareSerial PagWeb(1);

// ==== Declaración de variables
String JSON_entrada;
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;


void setup() {

  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");

  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);

  pinMode(SENSOR_PIN, INPUT);
  pinMode(RELAYCom, OUTPUT);

  digitalWrite(RELAYCom, LOW);

  Serial.println("Reading Qwiic module in digital mode...");
}

void loop() {

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "env") opc = 1;
      else if (Function == "IR") opc = 2;

      switch (opc) {
        case 1:
          {
            digitalWrite(RELAYCom, HIGH);
            sendJSON.clear();

            float val = 0;
            bool state = false;

            delay(100);
            for (int i = 0; i < 10; i++) {
              float lect = analogRead(SENSOR_PIN);
              Serial.println("El valor es: " + String(lect));
              val += lect;
              delay(50);
            }

            float avg = val / 10;

            if (avg > 2000) {
              Serial.println("Light detected (HIGH): " + String(avg));
              sendJSON["Env"] = "OK";
            } else {
              Serial.println("No light (LOW): " + String(avg));
              sendJSON["Env"] = "Fail";
            }

            // --- Conversión a porcentaje ---
            float porcentaje = (avg * 100.0) / 3100.0;

            // **Clamp (evitar pasar de 0-100%)**
            if (porcentaje < 0) porcentaje = 0;
            if (porcentaje > 100) porcentaje = 100;

            sendJSON["Meas_Env"] = String(porcentaje, 2) + " %";
            serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
            PagWeb.println();
            digitalWrite(RELAYCom, LOW);
            break;
          }

        case 2:
          {
            digitalWrite(RELAYCom, HIGH);
            sendJSON.clear();
            float suma = 0;

            delay(100);
            // --- Lectura filtrada ---
            for (int i = 0; i < 10; i++) {
              float lect = analogRead(SENSOR_PIN);
              Serial.println("El valor es: " + String(lect));
              suma += lect;
              delay(50);
            }

            float avg = suma / 10.0;

            // --- Lógica del sensor IR ---
            if (avg > 2000) {
              Serial.println("Light detected (HIGH): " + String(avg));
              sendJSON["IR"] = "OK";
            } else {
              Serial.println("No light (LOW): " + String(avg));
              sendJSON["IR"] = "Fail";
            }

            // --- Conversión a porcentaje ---
            float porcentaje = (avg * 100.0) / 3100.0;

            // **Clamp (evitar pasar de 0-100%)**
            if (porcentaje < 0) porcentaje = 0;
            if (porcentaje > 100) porcentaje = 100;

            sendJSON["Meas_IR"] = String(porcentaje, 2) + " %";

            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            digitalWrite(RELAYCom, LOW);
            break;
          }
      }
    }
  }
}