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
    else if (Function == "sweep") opc = 4;       // {"Function":"sweep"}
    else if (Function == "blink_in") opc = 5;    // {"Function":"blink_in"}

    // Funciones de Accionamiento de relevadores
    else if (Function == "scanAddr") opc = 6;   // {"Function": "scanAddr"}
    else if (Function == "channelON") opc = 7;  // {"Function": "channelON", "channel":1}
    else if (Function == "sleep") opc = 8;      // {"Function": "sleep"}


    // ==== DESPACHO POR TIPO DE OPERACIÓN ====
    switch (opc) {
      case 1:
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      case 2:  // ping_slave
        {
          serialDebug("Enviando PING hacia esclavo JUNR3...");

          // 1. Limpiar buffer y enviar el JSON de ping al esclavo
          sendJSON.clear();
          sendJSON["Function"] = "ping";
          serializeJson(sendJSON, Master);
          Master.println();  // Envía por UART a la JUNR3

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
              break;  // Salimos del bucle de espera inmediatamente
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
          sendJSON.clear();
          sendJSON["Function"] = "readSweep";
          serializeJson(sendJSON, Master);
          Master.println();
          delay(20);

          for (int i = 0; i < 10; i++) {
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

      case 5:  // blink_in (PULSAR lee la señal de la JUNR3)
        {
          serialDebug("Iniciando lectura Blink_in desde JUNR3...");

          // 1. Configurar pines de la PULSAR como entrada ADC
          for (int i = 0; i < NUM_GPIOS; i++) {
            pinMode(GPIOS[i], INPUT_PULLDOWN);
          }

          // 2. Avisar a la JUNR3 que empiece a mandar los pulsos
          sendJSON.clear();
          sendJSON["Function"] = "blink_in";
          serializeJson(sendJSON, Master);
          Master.println();

          // 3. Esperar confirmación sincrónica (BLINK_START)
          long waitStart = millis();
          bool isReady = false;
          while (millis() - waitStart < 2000) {  // Timeout de 2 segundos
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
            break;  // Abortar si el esclavo no responde
          }

          // 4. Máquina de estados para validar la señal analógica (ADC ESP32-C6)
          int pinStates[NUM_GPIOS] = { 0 };

          // ESP32-C6 a 3.3V
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

              // Fórmula para el ESP32-C6 (12-bits, 3.3V)
              float voltage = (raw_adc / 4095.0) * 3.3;

              if (voltage > maxVolts[i]) maxVolts[i] = voltage;
              if (voltage < minVolts[i]) minVolts[i] = voltage;

              int oldState = pinStates[i];

              // Umbrales calculados asumiendo que el divisor bajará los 5V a ~3.3V
              switch (pinStates[i]) {
                case 0:
                  if (voltage <= 0.8) pinStates[i] = 1;
                  break;
                case 1:
                  if (voltage >= 2.5) pinStates[i] = 2;  // Espera al menos 2.5V del divisor
                  break;
                case 2:
                  if (voltage <= 0.8) pinStates[i] = 3;
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

          // 5. Reporte de Debug en Serial de la PULSAR
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

          // 6. Imprimir el JSON resultante para la Interfaz Web
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
          serializeJson(sendJSON, Serial);  // Se manda a la PC/Web
          Serial.println();
          break;
        }


      case 6:  // Escaneo I2C
        {
          sendJSON.clear();
          for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
              String addrHex = "";
              if (addr < 16) addrHex = "0";
              addrHex = addrHex + String(addr, HEX);
              serialDebug("I2C device found at 0x" + addrHex);
            }
          }
          serialDebug("Scan complete...");
          break;
        }




      case 7:  // Activación de canal
        {
          if (channel >= 1 && channel <= 16) {
            serialDebug("Test Channel " + String(channel) + " ON...");

            // Mapear canal a comando I2C (0x00-0x0F para canales 1-16)
            switch (channel) {
              case 1: sendCommandI2C(0x00); break;
              case 2: sendCommandI2C(0x01); break;
              case 3: sendCommandI2C(0x02); break;
              case 4: sendCommandI2C(0x03); break;
              case 5: sendCommandI2C(0x04); break;
              case 6: sendCommandI2C(0x05); break;
              case 7: sendCommandI2C(0x06); break;
              case 8: sendCommandI2C(0x07); break;
              case 9: sendCommandI2C(0x08); break;
              case 10: sendCommandI2C(0x09); break;
              case 11: sendCommandI2C(0x0A); break;
              case 12: sendCommandI2C(0x0B); break;
              case 13: sendCommandI2C(0x0C); break;
              case 14: sendCommandI2C(0x0D); break;
              case 15: sendCommandI2C(0x0E); break;
              case 16: sendCommandI2C(0x0F); break;
              default: break;
            }
          } else {
            serialDebug("Invalid channel... Select [1-16]");
          }
          delay(100);
          break;
        }

      case 8:  // Modo suspensión
        {
          serialDebug("Sleep mode...");
          sendCommandI2C(0xFE);  // Comando de suspensión
          delay(100);
          break;
        }

      default:
        serialDebug("error option not allowed");
        break;
    }
  }
}
