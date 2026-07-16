/*
Firmware de prueba para la JUNR3 en modo esclavo.

Qué hace:
- Usa los pines analógicos como salidas digitales de alto/bajo.
- También puede leer señales analógicas mediante ADC para validar pulsos.
- Recibe órdenes por Serial en formato JSON desde la PULSARC6 o una interfaz externa.

Operaciones soportadas:
- ping: verifica comunicación básica.
- blink: prepara la JUNR3 para detectar pulsos y reporta resultados ADC.
- readSweep: ejecuta un barrido simple de lectura de voltaje por pin.
- blink_in: genera un pulso desde la JUNR3 hacia la PULSARC6.

Estructura del archivo:
1. Configuración de pines y bibliotecas.
2. Buffers y objetos JSON.
3. Variables globales y utilidades de depuración.
4. setup() para inicialización.
5. loop() con despacho por tipo de operación.
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

// ==== OBJETOS Y BUFFERS JSON ====
String JSON_entrada;                   ///< Buffer para recibir JSON desde la interfaz o la PULSARC6
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos
String JSON_salida;                    ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;     ///< Documento JSON para armar respuestas

// ==== VARIABLES GLOBALES ====
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
  // ==== CONFIGURACIÓN INICIAL ====
  Serial.begin(115200);
  serialDebug("Serial JUNR3 Initialized...");

  // ==== CONFIGURACIÓN DE PINES COMO SALIDAS POR DEFECTO ====
  for (int i = 0; i < NUM_GPIOS; i++) {
    pinMode(GPIOS[i], OUTPUT);
  }
}

void loop() {
  // ==== BUCLE PRINCIPAL / DESPACHADOR DE COMANDOS ====
  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // ==== DECODIFICACIÓN DE LA OPERACIÓN SOLICITADA ====
    String Function = receiveJSON["Function"];

    int opc = 0;
    if (Function == "ping") opc = 1;            // {"Function":"ping"}
    else if (Function == "blink") opc = 2;      // {"Function":"blink"}
    else if (Function == "readSweep") opc = 3;  // {"Function":"readSweep"}
    else if (Function == "blink_in") opc = 4;   // {"Function":"blink_in"}
    else if (Function == "blink_out") opc = 5;  // {"Function":"blink_out"}


    switch (opc) {

      // ===== COMUNICACIÓN BÁSICA =====
      // case 1: Ping de validación. Responde a la PULSARC6 con un mensaje JSON de confirmación.
      case 1:
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          sendJSON["debug"] = "Hello PULSARC6!";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      // ===== PRUEBAS DE BLINK / SEÑAL =====
      // case 2: Blink de entrada. Configura los pines como entradas y valida la señal recibida por ADC.
      case 2:
        {
          sendJSON.clear();
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
            // === REPORTE DE DEBUG ===
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
          sendJSON["Result"] = success ? "OK" : "FAIL";
          serializeJson(sendJSON, Serial);
          Serial.println();

          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], OUTPUT);
          }
          break;
        }

      // case 4: Blink de salida. Genera una secuencia de pulso desde la JUNR3 hacia la PULSARC6.
      case 4:
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

      // case 5: Secuencia de blink continuo. Alterna todos los GPIO de forma repetitiva para pruebas visuales.
      case 5:
        {
          Serial.println("Inicio del barrido...");

          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], OUTPUT);
          }
          delay(50);

          for (int i = 0; i < 10; i++) {
            digitalWrite(A0_PIN, LOW);
            digitalWrite(A1_PIN, LOW);
            digitalWrite(A2_PIN, LOW);
            digitalWrite(A3_PIN, LOW);
            digitalWrite(A4_PIN, LOW);
            digitalWrite(A5_PIN, LOW);
            delay(1000);
            digitalWrite(A0_PIN, HIGH);
            digitalWrite(A1_PIN, HIGH);
            digitalWrite(A2_PIN, HIGH);
            digitalWrite(A3_PIN, HIGH);
            digitalWrite(A4_PIN, HIGH);
            digitalWrite(A5_PIN, HIGH);
            delay(1000);
          }
          Serial.println("Finalizado");
          break;
        }

      // ===== BARRIDO / ADC =====
      // case 3: Barrido ADC. Lee el voltaje de todos los pines en forma repetitiva y lo imprime por Serial.
      case 3:
        {
          sendJSON.clear();

          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], INPUT_PULLUP);
          }

          // Damos un pequeño respiro para que la terminal no imprima basura inicial
          delay(50);
          Serial.println("--- INICIANDO BARRIDO RAW ADC Y VOLTAJE (ATmega 10-bit / 5V) ---");

          for (int i = 0; i < 100; i++) {
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
        }

      default:
        serialDebug("error option not allowed");
        break;
    }
  }
}
