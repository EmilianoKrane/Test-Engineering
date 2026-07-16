/*
Firmware de prueba para la PULSARC6 actuando como maestro del enlace.

Qué hace:
- Controla los pines analógicos como salidas digitales para generar pulsos.
- Comunica con la JUNR3 por UART2 usando JSON.
- Puede ejecutar pruebas de ping, blink_out, sweep y blink_in.

Operaciones soportadas:
- ping: verifica comunicación básica con la interfaz.
- ping_slave: consulta al esclavo JUNR3.
- blink_out: envía una orden de blink a la JUNR3 y espera validación.
- sweep: activa un barrido simple de los pines.
- blink_in: coordina la lectura de la señal enviada por la JUNR3.

Estructura del archivo:
1. Configuración de pines, UART y bibliotecas.
2. Buffers y objetos JSON.
3. Variables globales y utilidades de depuración.
4. setup() para inicialización.
5. loop() con despacho por tipo de operación.


Funcionamiento: 
- Passthrough: Se prueba con el case 3 el cual envia desde la PULSARC6 una señal de 3V3 al
 modulo convertidor level shifters y este lo eleva a 5V para su lectura desde una tarjeta JUNR3

- Boost Mode: El funcionamiento es igual al anterior solo que se debe retirar la alimentación
de la tarjeta JUNR3 y esta debe encender dada la salida del boost 

- Buck Mode: Se debe retirar la alimentación de la tarjeta pulsar y esta deberá encender
por la alimentación del buck
*/

// ==== BIBLIOTECAS ====
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <Wire.h>

// ==== DECLARACIÓN DE PINES ====
#define A0_PIN 0   // >> Analógico A0 - Salida digital y lectura con ADC
#define A1_PIN 1   // >> Analógico A1 - Salida digital y lectura con ADC
#define A2_PIN 2   // >> Analógico A2 - Salida digital y lectura con ADC
#define A3_PIN 3   // >> Analógico A3 - Salida digital y lectura con ADC
#define A4_PIN 4   // >> Analógico A4 - Salida digital y lectura con ADC
#define A5_PIN 5   // >> Analógico A5 - Salida digital y lectura con ADC
#define SDA_PIN 6  // >> SDA para I2C con el esclavo - Línea de datos I2C
#define SCL_PIN 7  // >> SCL para I2C con el esclavo - Línea de reloj I2C
#define RX2 15     // >> GPIO15 como RX de UART2 - Recepción de datos desde interfaz web
#define TX2 19     // >> GPIO19 como TX de UART2 - Transmisión de datos a interfaz web

// ==== OBJETOS Y BUFFERS JSON ====
HardwareSerial Master(1);              // Crear objeto para UART2 en PULSAR como bridge
String JSON_entrada;                   ///< Buffer para recibir JSON desde la interfaz o la PC
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos
String JSON_salida;                    ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;     ///< Documento JSON para armar respuesta

// ==== VARIABLES GLOBALES ====
const uint8_t SLAVE_ADDR = 0x40;  // Dirección I2C de tarjeta de relevadores
const int GPIOS[] = { A0_PIN, A1_PIN, A2_PIN, A3_PIN, A4_PIN, A5_PIN };
const int NUM_GPIOS = sizeof(GPIOS) / sizeof(GPIOS[0]);

// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  StaticJsonDocument<128> debug;
  debug["debug"] = str;
  serializeJson(debug, Serial);
  Serial.println();
}


// --- Función para enviar comando I2C al esclavo ---
/**
 * @brief Envía un comando I2C al dispositivo esclavo.
 * @param command Comando a enviar (byte).
 */
void sendCommandI2C(uint8_t command) {
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(command);
  uint8_t error = Wire.endTransmission();

  if (error == 0) {
    serialDebug("Comando enviado OK: 0x" + String(command, HEX));
  } else {
    serialDebug("Error I2C " + String(error) + " enviando: 0x" + String(command, HEX));
  }
}

// --- Función para leer respuesta del esclavo ---
/**
 * @brief Lee una respuesta del dispositivo esclavo vía I2C
 * @return Respuesta del esclavo (byte) o 0xFF en caso de error
 */
uint8_t readResponseI2C() {
  Wire.requestFrom(SLAVE_ADDR, (uint8_t)1);  // Solicitar 1 byte

  if (Wire.available()) {
    uint8_t response = Wire.read();
    serialDebug("Respuesta recibida: 0x" + String(response, HEX));
    return response;
  } else {
    serialDebug("No hubo respuesta del esclavo");
    return 0xFF;  // Valor de error arbitrario
  }
}


void setup() {
  // ==== CONFIGURACIÓN INICIAL ====
  Serial.begin(115200);
  Master.begin(115200, SERIAL_8N1, RX2, TX2);
  delay(100);
  serialDebug("Serial Pulsar C6 Initialized...");

  // ==== Inicialización de BUS I2C ====
  Wire.begin(SDA_PIN, SCL_PIN);  // Iniciar I2C como maestro
  serialDebug("I2C Maestro inicializado en SDA: " + String(SDA_PIN) + " SCL: " + String(SCL_PIN));
  Wire.setTimeOut(10000);

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
    int channel = receiveJSON["channel"] | 0;


    /*


    */

    int opc = 0;
    if (Function == "ping") opc = 1;  // {"Function":"ping"}

    // Funciones de activacion de gpios y lectura de tramas
    else if (Function == "ping_slave") opc = 2;  // {"Function":"ping_slave"}
    else if (Function == "blink_out") opc = 3;   // {"Function":"blink_out"}
    else if (Function == "blink_in") opc = 5;    // {"Function":"blink_in"}
    else if (Function == "sweep_out") opc = 4;   // {"Function":"sweep_out"}
    else if (Function == "sweep_in") opc = 10;   // {"Function":"sweep_in"}

    else if (Function == "testAll") opc = 6;           // {"Function": "testAll"}
    else if (Function == "passthrough_test") opc = 7;  // {"Function": "passthrough_test"}
    else if (Function == "buck_test") opc = 8;         // {"Function": "buck_test"}
    else if (Function == "boost_test") opc = 9;        // {"Function": "boost_test"}


    switch (opc) {
      // ===== COMUNICACIÓN BÁSICA =====
      // case 1: Ping de validación. Responde al monitor serie para confirmar que la PULSARC6 está operativa.
      case 1:
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      // ===== COMUNICACIÓN UART CON EL ESCLAVO JUNR3 =====
      // case 2: Ping hacia la JUNR3 por UART2. Verifica que el enlace entre ambas placas funciona.
      case 2:
        {
          serialDebug("Enviando PING hacia esclavo JUNR3...");

          sendJSON.clear();
          sendJSON["Function"] = "ping";
          serializeJson(sendJSON, Master);
          Master.println();

          unsigned long startWait = millis();
          bool responseReceived = false;

          while (millis() - startWait < 1500) {
            if (Master.available()) {
              String slaveResponse = Master.readStringUntil('\n');

              StaticJsonDocument<512> slaveDoc;
              DeserializationError error = deserializeJson(slaveDoc, slaveResponse);

              if (!error) {
                Serial.print("[UART-SLAVE-OK]: ");
                serializeJson(slaveDoc, Serial);
                Serial.println();
              } else {
                Serial.print("[UART-SLAVE-RAW]: ");
                Serial.println(slaveResponse);
              }

              responseReceived = true;
              break;
            }
          }

          if (!responseReceived) {
            serialDebug("Error: Timeout excedido. JUNR3 no responde.");
          }

          break;
        }

      // ===== PRUEBAS DE BLINK / SEÑAL =====
      // case 3: Blink de salida. La PULSARC6 ordena a la JUNR3 generar un pulso y valida la respuesta.
      case 3:
        {
          serialDebug("Iniciando secuencia Blink_out hacia JUNR3...");

          sendJSON.clear();
          sendJSON["Function"] = "blink";
          serializeJson(sendJSON, Master);
          Master.println();

          long waitStart = millis();
          bool isReady = false;
          while (millis() - waitStart < 2000) {
            if (Master.available()) {
              String rx = Master.readStringUntil('\n');
              StaticJsonDocument<256> rxDoc;
              deserializeJson(rxDoc, rx);
              if (rxDoc["status"] == "ADC_READY") {
                isReady = true;
                break;
              }
            }
          }

          if (!isReady) {
            serialDebug("Error: JUNR3 timeout o no respondió al handshake");
            break;
          }

          for (int i = 0; i < NUM_GPIOS; i++) digitalWrite(GPIOS[i], LOW);
          delay(100);

          for (int i = 0; i < NUM_GPIOS; i++) digitalWrite(GPIOS[i], HIGH);
          delay(500);

          for (int i = 0; i < NUM_GPIOS; i++) digitalWrite(GPIOS[i], LOW);

          waitStart = millis();
          bool validationReceived = false;
          while (millis() - waitStart < 3000) {
            if (Master.available()) {
              String rx = Master.readStringUntil('\n');
              Serial.println(rx);
              validationReceived = true;
              break;
            }
          }

          if (!validationReceived) {
            serialDebug("Error: JUNR3 no envió resultados de validación");
          }
          break;
        }

      // case 4: Sweep de salida. Activa un barrido de los GPIO para comprobar el estado del enlace.
      case 4:
        {
          sendJSON.clear();
          sendJSON["Function"] = "readSweep";
          serializeJson(sendJSON, Master);
          Master.println();
          delay(20);

          for (int i = 0; i < 20; i++) {
            digitalWrite(A0_PIN, LOW);
            digitalWrite(A1_PIN, LOW);
            digitalWrite(A2_PIN, LOW);
            digitalWrite(A3_PIN, LOW);
            digitalWrite(A4_PIN, LOW);
            digitalWrite(A5_PIN, LOW);
            delay(500);
            digitalWrite(A0_PIN, HIGH);
            digitalWrite(A1_PIN, HIGH);
            digitalWrite(A2_PIN, HIGH);
            digitalWrite(A3_PIN, HIGH);
            digitalWrite(A4_PIN, HIGH);
            digitalWrite(A5_PIN, HIGH);
            delay(500);
          }
          Serial.println("Finalizado");
          break;
        }

      // case 5: Blink de entrada. La PULSARC6 solicita a la JUNR3 que genere una señal y luego valida la lectura ADC.
      case 5:
        {
          serialDebug("Iniciando lectura Blink_in desde JUNR3...");

          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], INPUT_PULLDOWN);
          }

          sendJSON.clear();
          sendJSON["Function"] = "blink_in";
          serializeJson(sendJSON, Master);
          Master.println();

          long waitStart = millis();
          bool isReady = false;
          while (millis() - waitStart < 2000) {
            if (Master.available()) {
              String rx = Master.readStringUntil('\n');
              StaticJsonDocument<256> rxDoc;
              deserializeJson(rxDoc, rx);
              if (rxDoc["status"] == "BLINK_START") {
                isReady = true;
                break;
              }
            }
          }

          if (!isReady) {
            serialDebug("Error: JUNR3 timeout o no respondió al handshake de blink_in");
            break;
          }

          int pinStates[NUM_GPIOS] = { 0 };
          float maxVolts[NUM_GPIOS] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
          float minVolts[NUM_GPIOS] = { 3.3, 3.3, 3.3, 3.3, 3.3, 3.3 };

          unsigned long start_time = millis();
          bool allFinished = false;

          while (millis() - start_time < 2500 && !allFinished) {
            allFinished = true;
            for (int i = 0; i < NUM_GPIOS; i++) {
              if (pinStates[i] == 3) continue;
              allFinished = false;

              int raw_adc = analogRead(GPIOS[i]);
              float voltage = (raw_adc / 4095.0) * 3.3;

              if (voltage > maxVolts[i]) maxVolts[i] = voltage;
              if (voltage < minVolts[i]) minVolts[i] = voltage;

              int oldState = pinStates[i];

              switch (pinStates[i]) {
                case 0:
                  if (voltage <= 0.3) pinStates[i] = 1;
                  break;
                case 1:
                  if (voltage >= 2.1) pinStates[i] = 2;
                  break;
                case 2:
                  if (voltage <= 0.3) pinStates[i] = 3;
                  break;
              }

              if (pinStates[i] != oldState) {
                Serial.print("[DEBUG] ESP ADC GPIO ");
                Serial.print(GPIOS[i]);
                Serial.print(" paso de ");
                Serial.print(oldState);
                Serial.print(" a ");
                Serial.print(pinStates[i]);
                Serial.print(" con: ");
                Serial.print(voltage);
                Serial.println("V");
              }
            }
          }

          Serial.println("\n--- REPORTE DE VOLTAJES MAX/MIN (ESP32-C6) ---");
          for (int i = 0; i < NUM_GPIOS; i++) {
            Serial.print("GPIO ");
            Serial.print(GPIOS[i]);
            Serial.print(" -> Min: ");
            Serial.print(minVolts[i]);
            Serial.print("V | Max: ");
            Serial.print(maxVolts[i]);
            Serial.print("V | Estado: ");
            Serial.println(pinStates[i]);
          }

          sendJSON.clear();
          sendJSON["Function"] = "blink_in_result";
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
          break;
        }


      // ===== PRUEBAS DE PASSTHROUGH / SECUENCIAS COMBINADAS =====
      // case 6: Secuencia de test combinado. Coordina el blink hacia la JUNR3 y deja preparado el bloque para futuras extensiones.
      case 6:
        {
          sendJSON.clear();
          serialDebug("Iniciando secuencia Blink_out hacia JUNR3...");

          sendJSON.clear();
          sendJSON["Function"] = "blink";
          serializeJson(sendJSON, Master);
          Master.println();
          delay(20);

          long waitStart = millis();
          bool isReady = false;
          while (millis() - waitStart < 2000) {
            if (Master.available()) {
              String rx = Master.readStringUntil('\n');
              StaticJsonDocument<256> rxDoc;
              deserializeJson(rxDoc, rx);
              if (rxDoc["status"] == "ADC_READY") {
                isReady = true;
                break;
              }
            }
          }

          if (!isReady) {
            serialDebug("Error: JUNR3 timeout o no respondió al handshake");
          }

          for (int i = 0; i < NUM_GPIOS; i++) digitalWrite(GPIOS[i], LOW);
          delay(100);

          for (int i = 0; i < NUM_GPIOS; i++) digitalWrite(GPIOS[i], HIGH);
          delay(500);

          for (int i = 0; i < NUM_GPIOS; i++) digitalWrite(GPIOS[i], LOW);

          waitStart = millis();
          bool validationReceived = false;
          while (millis() - waitStart < 3000) {
            if (Master.available()) {
              String rx = Master.readStringUntil('\n');
              Serial.println(rx);
              validationReceived = true;
              break;
            }
          }

          if (!validationReceived) {
            serialDebug("Error: JUNR3 no envió resultados de validación");
          }

          break;
        }

      case 7:
        {
          break;
        }

      case 8:
        {
          break;
        }

      case 9:
        {
          break;
        }







      // ===== BARRIDO / ADC DE ENTRADA =====
      // case 10: Sweep de entrada. Ejecuta un barrido analógico y lo reporta por Serial para diagnóstico.
      case 10:
        {
          sendJSON.clear();
          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], INPUT_PULLDOWN);
          }
          delay(20);

          sendJSON["Function"] = "blink_out";
          serializeJson(sendJSON, Master);
          Master.println();

          delay(50);
          Serial.println("--- INICIANDO BARRIDO RAW ADC Y VOLTAJE (ATmega 10-bit / 5V -> 3V3) ---");

          for (int i = 0; i < 100; i++) {
            for (int j = 0; j < NUM_GPIOS; j++) {
              int raw_adc = analogRead(GPIOS[j]);
              float voltage = (raw_adc / 4095.0) * 3.3;

              Serial.print("A");
              Serial.print(j);
              Serial.print(": ");
              Serial.print(raw_adc);
              Serial.print(" (");
              Serial.print(voltage, 2);
              Serial.print("V)");

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






      default:
        serialDebug("error option not allowed");
        break;
    }
  }
}
