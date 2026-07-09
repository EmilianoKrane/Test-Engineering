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
StaticJsonDocument<1024> sendJSON;     ///< Documento JSON para armar respuestas

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
  serialDebug("Serial JUNR3 Initialized...");

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
    if (Function == "ping") opc = 1;            // {"Function":"ping"}
    else if (Function == "blink") opc = 2;      // {"Function":"blink"}
    else if (Function == "readSweep") opc = 3;  // {"Function":"readSweep"}
    else if (Function == "blink_in") opc = 4;   // {"Function":"blink_in"}

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

      case 2:  // blink (Recepción ADC con Debug)
        {
          bool debug = false;

          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], INPUT_PULLUP);
          }

          sendJSON.clear();
          sendJSON["status"] = "ADC_READY";
          serializeJson(sendJSON, Serial);
          Serial.println();

          int pinStates[NUM_GPIOS] = { 0 };

          // Arreglos para trackear voltajes pico y valles
          float maxVolts[NUM_GPIOS] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
          float minVolts[NUM_GPIOS] = { 5.0, 5.0, 5.0, 5.0, 5.0, 5.0 };

          unsigned long start_time = millis();
          bool allFinished = false;

          while (millis() - start_time < 2500 && !allFinished) {
            allFinished = true;
            for (int i = 0; i < NUM_GPIOS; i++) {
              if (pinStates[i] == 3) continue;
              allFinished = false;

              int raw_adc = analogRead(GPIOS[i]);

              // Conversión ajustada a 10-bits (1023.0) y 5.0V
              float voltage = (raw_adc / 1023.0) * 5.0;

              // Guardar el máximo y mínimo histórico de este pin
              if (voltage > maxVolts[i]) maxVolts[i] = voltage;
              if (voltage < minVolts[i]) minVolts[i] = voltage;

              int oldState = pinStates[i];  // Guardamos el estado actual para comparar

              switch (pinStates[i]) {
                case 0:
                  if (voltage <= 0.9) pinStates[i] = 1;
                  break;
                case 1:
                  if (voltage >= 3.2) pinStates[i] = 2;
                  break;
                case 2:
                  if (voltage <= 0.9) pinStates[i] = 3;
                  break;
              }

              if (debug) {
                // Imprimir SOLO si hubo un cambio de estado (no bloquea el loop)
                if (pinStates[i] != oldState) {
                  Serial.print("[DEBUG] Pin GPIO ");
                  Serial.print(GPIOS[i]);
                  Serial.print(" paso de estado ");
                  Serial.print(oldState);
                  Serial.print(" a ");
                  Serial.print(pinStates[i]);
                  Serial.print(" con un voltaje de: ");
                  Serial.println(voltage);
                }
              }
            }
          }

          if (debug) {
            // === REPORTE DE DEBUG POST-MORTEM ===
            Serial.println("\n--- REPORTE DE VOLTAJES MAX/MIN ---");
            for (int i = 0; i < NUM_GPIOS; i++) {
              Serial.print("GPIO ");
              Serial.print(GPIOS[i]);
              Serial.print(" -> Min: ");
              Serial.print(minVolts[i]);
              Serial.print("V | Max: ");
              Serial.print(maxVolts[i]);
              Serial.print("V | Estado final: ");
              Serial.println(pinStates[i]);
            }
            Serial.println("-----------------------------------\n");
          }

          // JSON de respuesta normal
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
          serializeJson(sendJSON, Serial);
          Serial.println();

          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], OUTPUT);
          }
          break;
        }

      case 3:
        {
          sendJSON.clear();

          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], INPUT_PULLUP);
          }

          // Damos un pequeño respiro para que la terminal no imprima basura inicial
          delay(50);
          Serial.println("--- INICIANDO BARRIDO RAW ADC Y VOLTAJE (ATmega 10-bit / 5V) ---");

          for (int i = 0; i < 50; i++) {
            // Iteramos sobre los pines para que el código quede impecable y escalable
            for (int j = 0; j < NUM_GPIOS; j++) {
              int raw_adc = analogRead(GPIOS[j]);

              // Conversión con la fórmula correcta para el ATmega328P
              float voltage = (raw_adc / 1023.0) * 5.0;

              Serial.print("A");
              Serial.print(j);
              Serial.print(": ");
              Serial.print(raw_adc);
              Serial.print(" (");
              Serial.print(voltage, 2);  // El '2' le dice que imprima solo dos decimales (ej. 3.60V)
              Serial.print("V)");

              // Si no es el último pin, imprimimos el separador. Si es el último, damos el salto de línea.
              if (j < NUM_GPIOS - 1) {
                Serial.print(" | ");
              } else {
                Serial.println();
              }
            }
            delay(100);
          }

          Serial.println("--- BARRIDO FINALIZADO ---");
          break;
        }

      case 4:  // blink_in (JUNR3 genera los pulsos hacia la PULSAR)
        {
          // 1. Configurar los pines analógicos del ATmega como salidas digitales
          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], OUTPUT);
            digitalWrite(GPIOS[i], LOW);  // Forzar el estado bajo inicial
          }

          // 2. Avisar a la PULSARC6 que el hardware está listo para empezar
          sendJSON.clear();
          sendJSON["status"] = "BLINK_START";
          serializeJson(sendJSON, Serial);
          Serial.println();

          // Le damos 50ms a la PULSAR para que entre cómodamente a su ciclo while() de lectura
          delay(50);

          // 3. Ejecutar la secuencia de pulsos (0V -> 5V -> 0V)
          for (int i = 0; i < NUM_GPIOS; i++) digitalWrite(GPIOS[i], HIGH);
          delay(500);  // Mantenemos el pulso alto por medio segundo

          for (int i = 0; i < NUM_GPIOS; i++) digitalWrite(GPIOS[i], LOW);

          // La JUNR3 no necesita mandar reporte de validación, de eso ya se encargó la PULSAR.
          break;
        }

      default:
        serialDebug("error option not allowed");
        break;
    }
  }
}
