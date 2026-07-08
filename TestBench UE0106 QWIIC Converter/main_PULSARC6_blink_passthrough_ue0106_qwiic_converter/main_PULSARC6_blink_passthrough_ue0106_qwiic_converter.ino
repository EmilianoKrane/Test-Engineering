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
#define A0_PIN 0   // >> Analógico A0 - Salida digital y lectura con ADC
#define A1_PIN 1   // >> Analógico A1 - Salida digital y lectura con ADC
#define A2_PIN 3   // >> Analógico A2 - Salida digital y lectura con ADC
#define A3_PIN 4   // >> Analógico A3 - Salida digital y lectura con ADC
#define A4_PIN 22  // >> Analógico A4 - Salida digital y lectura con ADC
#define A5_PIN 23  // >> Analógico A5 - Salida digital y lectura con ADC
#define RX2 15     // >> GPIO15 como RX de UART2 - Recepción de datos desde interfaz web
#define TX2 19     // >> GPIO19 como TX de UART2 - Transmisión de datos a interfaz web

// ==== CREACIÓN DE OBJETOS ====
HardwareSerial Master(1);              // Crear objeto para UART2 en PULSAR como Bridge
String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos
String JSON_salida;                    ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;     ///< Documento JSON para armar respuesta

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
  Master.begin(115200, SERIAL_8N1, RX2, TX2);
  delay(100);
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
    if (Function == "ping") opc = 1;             // {"Function":"ping"}
    else if (Function == "ping_slave") opc = 2;  // {"Function":"ping_slave"}
    else if (Function == "blink_out") opc = 3;   // {"Function":"blink_out"}
    else if (Function == "blink_in") opc = 4;    // {"Function":"blink_in"}

    switch (opc) {
      case 1:
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

case 2: // ping_slave
        {
          serialDebug("Enviando PING hacia esclavo JUNR3...");

          // 1. Limpiar buffer y enviar el JSON de ping al esclavo
          sendJSON.clear();
          sendJSON["Function"] = "ping";
          serializeJson(sendJSON, Master);
          Master.println(); // Envía por UART a la JUNR3

          // 2. Bloque de escucha con Timeout no bloqueante
          unsigned long startWait = millis();
          bool responseReceived = false;

          // Esperamos hasta 1.5 segundos por la respuesta del esclavo
          while (millis() - startWait < 1500) {
            if (Master.available()) {
              // Leer la línea completa que responde la JUNR3
              String slaveResponse = Master.readStringUntil('\n');
              
              // Intentar parsear para asegurar que el JSON es válido (opcional, pero buena práctica)
              StaticJsonDocument<512> slaveDoc;
              DeserializationError error = deserializeJson(slaveDoc, slaveResponse);

              if (!error) {
                // Si el JSON es correcto, imprimimos un log limpio y estructurado en el Serial principal
                Serial.print("[UART-SLAVE-OK]: ");
                serializeJson(slaveDoc, Serial);
                Serial.println();
              } else {
                // Si llegó texto plano o un JSON corrupto, lo mostramos como texto crudo
                Serial.print("[UART-SLAVE-RAW]: ");
                Serial.println(slaveResponse);
              }

              responseReceived = true;
              break; // Salimos del bucle de espera inmediatamente
            }
          }

          // 3. Alerta en caso de que el esclavo esté desconectado o sin energía
          if (!responseReceived) {
            serialDebug("Error: Timeout excedido. JUNR3 no responde.");
          }
          
          break;
        }

      case 3:  // blink_out
        {
          serialDebug("Iniciando secuencia Blink_out hacia JUNR3...");

          // 1. Avisar a la JUNR3 que se configure como ADC
          sendJSON.clear();
          sendJSON["Function"] = "blink";
          serializeJson(sendJSON, Master);
          Master.println();

          // 2. Esperar confirmación sincrónica (ADC_READY)
          long waitStart = millis();
          bool isReady = false;
          while (millis() - waitStart < 2000) {  // Timeout de 2 segundos
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
            break;  // Abortar si no hay respuesta
          }

          // 3. Ejecutar los pulsos (0V -> 3.3V -> 0V)
          // Nos aseguramos que inicie en LOW
          for (int i = 0; i < NUM_GPIOS; i++) digitalWrite(GPIOS[i], LOW);
          delay(100);

          // Flanco de subida
          for (int i = 0; i < NUM_GPIOS; i++) digitalWrite(GPIOS[i], HIGH);
          delay(500);  // Tiempo en estado alto

          // Flanco de bajada
          for (int i = 0; i < NUM_GPIOS; i++) digitalWrite(GPIOS[i], LOW);

          // 4. Esperar el reporte de validación final de la JUNR3
          waitStart = millis();
          bool validationReceived = false;
          while (millis() - waitStart < 3000) {  // Timeout de 3 segundos para los resultados
            if (Master.available()) {
              String rx = Master.readStringUntil('\n');
              // Reenviar el JSON exacto a la interfaz web
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

      case 4:
        {

          break;
        }


      default:
        serialDebug("error option not allowed");
        break;
    }
  }
}
