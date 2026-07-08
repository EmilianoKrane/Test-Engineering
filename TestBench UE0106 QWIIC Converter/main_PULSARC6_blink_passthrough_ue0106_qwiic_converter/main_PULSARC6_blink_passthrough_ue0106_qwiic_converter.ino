/*
>> Firmware JUNR3 Blink 5V -> recibe PULSARC6 a través de un divisor de tensión para regular a 3.3V
Este firmware se encarga de utilizar los gpios analogicos como salidas digitales de 1 y 0, y a su vez, realiza una
 lectura de entrada analogica. 
 La JUNR3 se usa como esclavo y recibe instrucciones por medio de uart 
*/

// ==== BIBLIOTECAS ====
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

// ==== DECLARACIÓN DE PINES ====
#define A0_PIN 14  // >> Analógico A0 - Salida digital y lectura con ADC
#define A1_PIN 15  // >> Analógico A1 - Salida digital y lectura con ADC
#define A2_PIN 16  // >> Analógico A2 - Salida digital y lectura con ADC
#define A3_PIN 17  // >> Analógico A3 - Salida digital y lectura con ADC
#define A4_PIN 18  // >> Analógico A4 - Salida digital y lectura con ADC
#define A5_PIN 19  // >> Analógico A5 - Salida digital y lectura con ADC

// ==== CREACIÓN DE OBJETOS ====
String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_salida;                 ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestass

// ==== CREACIÓN DE VARIABLES GLOBALES ====
const int GPIOS[] = { A0_PIN, A1_PIN, A2_PIN, A3_PIN, A4_PIN, A5_PIN };
const int NUM_GPIOS = sizeof(GPIOS) / sizeof(GPIOS[0]);

// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  StaticJsonDocument<128> debug;
  debug["debug"] = str;
  serializeJson(debug, Serial);
  Serial.println();
}

void setup() {

  Serial.begin(115200);
  serialDebug("Serial Initialized...");

  // ==== Iteración sobre los gpios declarados para definirlos como salidas ====
  for (int i = 0; i < NUM_GPIOS; i++) {
    pinMode(GPIOS[i], OUTPUT);
  }
}

void loop() {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // ==== Creación de claves de entrada admitidas por JSON ====
    String Function = receiveJSON["Function"];

    int opc = 0;
    if (Function == "ping") opc = 1;        // {"Function":"ping"}
    else if (Function == "blink") opc = 2;  // {"Function":"blink"}

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

          break;
        }


      default:
        serialDebug("error option not allowed");
        break;
    }
  }
}
