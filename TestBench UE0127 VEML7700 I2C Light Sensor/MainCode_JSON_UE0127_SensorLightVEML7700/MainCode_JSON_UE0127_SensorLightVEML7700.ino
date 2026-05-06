/*
  Firmware principal para el TestBench UE0127: sensor de luz VEML7700.

  Funcionalidad:
    - Comunicación I2C con el sensor VEML7700 usando la librería Wire.
    - Interfaz UART2 para recibir órdenes JSON y responder con datos JSON.
    - Control de relé USB 5V mediante GPIO20.
    - Lectura de botón de arranque en GPIO4.

  Conexiones principales:
    - SDA -> GPIO6
    - SCL -> GPIO7
    - UART2 RX -> GPIO15
    - UART2 TX -> GPIO19
    - Relé USB -> GPIO20

  Notas:
    - Se usa el puerto serie PagWeb (UART2) como enlace de comunicación con el frontend.
    - Si se desea usar el Serial USB nativo, el código debe adaptarse para usar Serial
      en lugar de PagWeb y la conexión física del testbench.
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include "Adafruit_VEML7700.h"

// ==== DEFINICIONES DE PINS ====
#define RUN_BUTTON 4  // Botón de arranque del testbench
#define SDA_PIN 6     // SDA para I2C con el sensor VEML7700
#define SCL_PIN 7     // SCL para I2C con el sensor VEML7700
#define RX2_PIN 15    // RX UART2 hacia el frontend PagWeb
#define TX2_PIN 19    // TX UART2 hacia el frontend PagWeb
#define RELAYUSB 20   // Relé USB 5V para el arnés de iluminación

// ==== CREACIÓN DE OBJETOS ====
HardwareSerial PagWeb(1);
Adafruit_VEML7700 veml = Adafruit_VEML7700();

String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;                ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas


/**
 * Envía mensajes de depuración al puerto serie USB nativo.
 */
void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas para JSON válido
  Serial.println("{\"debug\": \"" + str + "\"}");
}

/**
 * Envía mensajes de depuración al puerto UART2 PagWeb.
 */
void pagwebDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas para JSON válido
  PagWeb.println("{\"debug\": \"" + str + "\"}");
}

void setup() {

  // ==== Inicialización de comunicación serial ====
  Serial.begin(115200);
  PagWeb.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  delay(100);
  serialDebug("Serial Initialized...");
  pagwebDebug("Test Initialized...");

  // ==== Inicialización del bus I2C ====
  Wire.begin(SDA_PIN, SCL_PIN);

  // ==== Configuración de pines ====
  pinMode(RUN_BUTTON, INPUT);
  pinMode(RELAYUSB, OUTPUT);
  digitalWrite(RELAYUSB, LOW);
}

void loop() {

  // Comprueba si el botón de arranque se ha pulsado y responde con un JSON de aceptación.
  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);
    sendJSON.clear();  // Limpia cualquier dato previo

    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Arranque por botonera");
      sendJSON["Run"] = "OK";            // Envío de señal de arranque válido
      serializeJson(sendJSON, PagWeb);      // Envío de datos por JSON a la PagWeb
      PagWeb.println();
    }
  }

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');                              // Lee el JSON completo hasta el salto de línea
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Parsea el JSON recibido

    // ==== Extracción de parámetros de la petición JSON ====
    String Function = receiveJSON["Function"];
    int Gain = receiveJSON["Gain"] | 3;
    int IntTime = receiveJSON["IntTime"] | 100;

    int opc = 0;
    if (Function == "ping") opc = 1;             // {"Function":"ping"}
    else if (Function == "scanDis") opc = 2;     // {"Function":"scanDis"}
    else if (Function == "initSensor") opc = 3;  // {"Function":"initSensor"}
    else if (Function == "setSensor") opc = 4;   // {"Function":"setSensor", "Gain":1, "IntTime": 25}
    else if (Function == "readSensor") opc = 5;  // {"Function":"readSensor"}
    else if (Function == "relayON") opc = 6;     // {"Function":"relayON"}
    else if (Function == "relayOFF") opc = 7;    // {"Function":"relayOFF"}

    switch (opc) {
      case 1:  // -> Respuesta UART puente frontend <-> testbench
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 2:  // -> Escáner de Dispositivos I2C
        {
          sendJSON.clear();
          for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
              String addrHex = "";
              if (addr < 16) addrHex = "0";
              addrHex = addrHex + String(addr, HEX);
              serialDebug("I2C device found at 0x" + addrHex);
              pagwebDebug("I2C device found at 0x" + addrHex);
            }
          }
          break;
        }

      case 3:
        {
          sendJSON.clear();
          if (!veml.begin(&Wire)) serialDebug("Sensor no encontrado. Revisa la conexión.");
          else {
            serialDebug("¡Sensor VEML7700 encontrado y listo!");
            pagwebDebug("Sensor VEML7700 initialized...");
            sendJSON["Result"] = "OK";
          }
          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 4:
        {
          /* Gain:                          Integration time:    
              1 = VEML7700_GAIN_1_8         25  = VEML7700_IT_25MS
              2 = VEML7700_GAIN_1_4         50  = VEML7700_IT_50MS
              3 = VEML7700_GAIN_1           100 = VEML7700_IT_100MS
              4 = EML7700_GAIN_2            200 = VEML7700_IT_200MS
                                            400 = VEML7700_IT_400MS
                                            800 = VEML7700_IT_800MS
          */

          sendJSON.clear();
          // ==== Establecemos parámetros de configuración ====
          switch (Gain) {
            case 1: veml.setGain(VEML7700_GAIN_1_8); break;
            case 2: veml.setGain(VEML7700_GAIN_1_4); break;
            case 3: veml.setGain(VEML7700_GAIN_1); break;
            case 4: veml.setGain(VEML7700_GAIN_2); break;
            default: break;
          }
          switch (IntTime) {
            case 25: veml.setIntegrationTime(VEML7700_IT_25MS); break;
            case 50: veml.setIntegrationTime(VEML7700_IT_50MS); break;
            case 100: veml.setIntegrationTime(VEML7700_IT_100MS); break;
            case 200: veml.setIntegrationTime(VEML7700_IT_200MS); break;
            case 400: veml.setIntegrationTime(VEML7700_IT_400MS); break;
            case 800: veml.setIntegrationTime(VEML7700_IT_800MS); break;
            default: break;
          }
          delay(50);

          // ==== Confirmación de parámetros ====
          switch (veml.getGain()) {
            case VEML7700_GAIN_1: pagwebDebug("Gain 1"); break;
            case VEML7700_GAIN_2: pagwebDebug("Gain 2"); break;
            case VEML7700_GAIN_1_4: pagwebDebug("Gain 1/4"); break;
            case VEML7700_GAIN_1_8: pagwebDebug("Gain 1/8"); break;
          }
          switch (veml.getIntegrationTime()) {
            case VEML7700_IT_25MS: pagwebDebug("Time 25"); break;
            case VEML7700_IT_50MS: pagwebDebug("Time 50"); break;
            case VEML7700_IT_100MS: pagwebDebug("Time 100"); break;
            case VEML7700_IT_200MS: pagwebDebug("Time 200"); break;
            case VEML7700_IT_400MS: pagwebDebug("Time 400"); break;
            case VEML7700_IT_800MS: pagwebDebug("Time 800"); break;
          }

          // serialDebug(" Gain: " + String(veml.getGain()) + " | TimeInt: " + String(veml.getIntegrationTime()));
          veml.setLowThreshold(10000);
          veml.setHighThreshold(20000);
          veml.interruptEnable(true);
          break;
        }

      case 5:
        {
          sendJSON.clear();
          int samples = 10, delay_ms = 50;
          float avgLW = 0, avgLUX = 0, white = 0, lux = 0;
          constexpr bool debug = false;

          // ==== Debug de Lecturas en Monitor Serie y Promedio ====
          for (int i = 0; i < samples; i++) {
            white = veml.readWhite();
            lux = veml.readLux();

            if (debug) {
              Serial.print("Luz Bruta (ALS): ");
              Serial.print(veml.readALS());
              Serial.print("\tLuz Blanca: ");
              Serial.print(white);
              Serial.print("\tLux calculados: ");  // El VEML7700 procesa lux reales basados en un cálculo interno
              Serial.print(lux);
              Serial.println();
            }

            avgLW += white;
            avgLUX += lux;
            delay(delay_ms);
          }

          avgLW /= samples;
          avgLUX /= samples;
          sendJSON["white"] = avgLW;
          sendJSON["lux"] = avgLUX;
          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 6:
        {
          sendJSON.clear();
          digitalWrite(RELAYUSB, HIGH);
          pagwebDebug("Relay ON...");
          break;
        }

      case 7:
        {
          sendJSON.clear();
          digitalWrite(RELAYUSB, LOW);
          pagwebDebug("Relay OFF...");
          break;
        }


      default: break;
    }
  }
}
