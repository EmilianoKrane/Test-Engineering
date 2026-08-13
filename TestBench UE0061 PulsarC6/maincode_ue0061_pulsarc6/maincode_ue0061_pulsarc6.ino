/*
ue0061 firmware test main pulsar c6
*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <WiFi.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// ==== DECLARACIÓN DE GPIOS ====
#define SDA_PIN 6  // >> GPIO06 Señal de datos en protocolo I2C
#define SCL_PIN 7  // >> GPIO07 Señal de reloj en protocolo I2C


// ==== DECLARACIÓN DE VARIABLES GLOBALES ====

// ==== CREACIÓN DE OBJETOS ====
String JSON_entrada;
StaticJsonDocument<200> receiveJSON;
String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;



// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  sendJSON.clear();
  sendJSON["debug"] = str;
  serializeJson(sendJSON, Serial);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(100);
  sendJSON["state"] = "ready";
  sendJSON["test"] = "pulsarc6";

  Wire.begin(SDA_PIN, SCL_PIN);
  WiFi.mode(WIFI_MODE_STA);


  serializeJson(sendJSON, Serial);
  Serial.println();
}

void loop() {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "ping") opc = 1;            // {"Function":"ping"}
      else if (Function == "mac") opc = 2;        // {"Function":"mac"}
      else if (Function == "neop_test") opc = 5;  // {"Function":"neop_test"}


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
            String mac = WiFi.macAddress();
            sendJSON["mac"] = mac;
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }



        case 5:
          {
            break;
          }

        default:
          {
            sendJSON.clear();
            sendJSON["error"] = "invalid option";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  }
}
