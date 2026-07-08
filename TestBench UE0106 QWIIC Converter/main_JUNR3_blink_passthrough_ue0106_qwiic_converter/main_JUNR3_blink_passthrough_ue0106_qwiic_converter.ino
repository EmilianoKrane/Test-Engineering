/*
>> Firmware JUNR3 Blink 5V -> recibe PULSARC6 a través de un divisor de tensión para regular a 3.3V
Este firmware se encarga de utilizar los gpios analogicos como salidas digitales de 1 y 0, y a su vez, realiza una
 lectura de entrada analogica. 
 La JUNR3 se usa como esclavo y recibe instrucciones por medio de uart a través de los gpios 0 y 1
*/

// ==== BIBLIOTECAS ====
#include <Arduino.h>
#include <ArduinoJson.h>

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
String JSON_salida;                    ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;     ///< Documento JSON para armar respuestass

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
          sendJSON["debug"] = "Hello PULSARC6!";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      case 2:  // blink (Recepción ADC)
        {
          // 1. Configurar pines como entrada (alta impedancia para el divisor)
          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], INPUT);
          }

          // 2. Avisar a la PULSARC6 que el ADC está listo
          sendJSON.clear();
          sendJSON["status"] = "ADC_READY";
          serializeJson(sendJSON, Serial);
          Serial.println();

          // 3. Máquina de estados para validar la señal analógica
          int pinStates[NUM_GPIOS] = { 0 };
          unsigned long start_time = millis();
          bool allFinished = false;

          // Ventana de tiempo de 2.5 segundos para capturar todos los blinks
          while (millis() - start_time < 2500 && !allFinished) {
            allFinished = true;
            for (int i = 0; i < NUM_GPIOS; i++) {
              if (pinStates[i] == 3) continue;  // Si este pin ya validó todo, lo saltamos
              allFinished = false;

              int raw_adc = analogRead(GPIOS[i]);
              // Conversión asumiendo un ADC de 12-bits (ej. ESP32/RP2040)
              float voltage = (raw_adc / 4095.0) * 3.3;

              // Umbrales tolerantes para compensar caídas en el divisor de tensión
              switch (pinStates[i]) {
                case 0:  // Esperando asegurar que arranca en estado bajo (< 0.5V)
                  if (voltage <= 0.5) pinStates[i] = 1;
                  break;
                case 1:  // Esperando el flanco de subida (> 2.8V)
                  if (voltage >= 2.8) pinStates[i] = 2;
                  break;
                case 2:                                  // Esperando el flanco de bajada (< 0.5V)
                  if (voltage <= 0.5) pinStates[i] = 3;  // ¡Secuencia validada uwu!
                  break;
              }
            }
          }

          // 4. Preparar JSON con el dictamen de los pines
          sendJSON.clear();
          sendJSON["Function"] = "blink_result";
          JsonArray results = sendJSON.createNestedArray("pins_status");
          bool success = true;

          for (int i = 0; i < NUM_GPIOS; i++) {
            if (pinStates[i] == 3) {
              results.add("OK");
            } else {
              results.add("FAIL");
              success = false;
            }
          }

          sendJSON["overall"] = success ? "SUCCESS" : "ERROR";

          // 5. Enviar resultados de vuelta a la PULSAR
          serializeJson(sendJSON, Serial);
          Serial.println();

          // Restaurar pines a salidas por si el programa requiere volver al estado original
          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], OUTPUT);
          }
          break;
        }


      default:
        serialDebug("error option not allowed");
        break;
    }
  }
}
