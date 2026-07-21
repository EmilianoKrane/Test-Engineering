#include <Wire.h>
#include "DevLab_MAX30102.h"
#include "spo2_algorithm.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

// ==== DECLARACIÓN DE PINES ====
#define RUN_BUTTON 4  // >> Botonera de Arranque
#define SDA_PIN 6     // >> SDA para I2C
#define SCL_PIN 7     // >> SCL para I2C
#define RX2 15        // >> GPIO15 como RX de UART2 - Recepción de datos desde interfaz web
#define TX2 19        // >> GPIO19 como TX de UART2 - Transmisión de datos a interfaz web

// ==== CREACIÓN DE OBJETOS ====
HardwareSerial PagWeb(1);  // Crear objeto para UART2 en PULSAR como PagWeb
MAX30105 sensor;
StaticJsonDocument<1024> receiveJSON;
StaticJsonDocument<1024> sendJSON;

// ==== VARIABLES DE ESTADO Y SEGURIDAD ====
bool sensorInitialized = false;  // CANDADO 1: Verifica si el sensor arrancó
#define SPO2_SAMPLES 100
const byte RATE_SIZE = 8;
float rates[RATE_SIZE];
byte rateSpot = 0;
byte validRates = 0;

float bpmInst = 0;
float bpmAvg = 0;
unsigned long lastBeatTime = 0;
bool wasAbove = false;

float irDC = 0;
float pulseSignal = 0;
float threshold = 80;

// SpO2 measurement
uint32_t irBuffer[SPO2_SAMPLES];
uint32_t redBuffer[SPO2_SAMPLES];
byte spo2Index = 0;

int32_t spo2;
int8_t validSPO2;
int32_t dummyHR;
int8_t dummyValidHR;

float spo2Avg = 0;
int lastValidSpO2 = 0;
bool spo2Ready = false;

// Signal thresholds
const long FINGER_THRESHOLD = 8000;
const long SIGNAL_LOW = 25000;
const long SIGNAL_GOOD_MAX = 110000;

// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  sendJSON.clear();
  sendJSON["debug"] = str;
  serializeJson(sendJSON, Serial);
  Serial.println();
}

// ==== LÓGICA MATEMÁTICA (Extraída para modularidad) ====
void processVitalSigns(long irValue, long redValue) {
  if (irDC == 0) irDC = irValue;

  irDC = (irDC * 0.95) + (irValue * 0.05);
  pulseSignal = irValue - irDC;

  float absSignal = fabs(pulseSignal);
  threshold = (threshold * 0.95) + (absSignal * 0.05);
  bool isAbove = pulseSignal > threshold * 0.6;

  if (isAbove && !wasAbove) {
    unsigned long now = millis();
    unsigned long delta = now - lastBeatTime;

    if (delta > 300 && delta < 1500) {
      bpmInst = 60000.0 / delta;

      if (bpmInst >= 45 && bpmInst <= 160) {
        rates[rateSpot++] = bpmInst;
        rateSpot %= RATE_SIZE;
        if (validRates < RATE_SIZE) validRates++;

        float sum = 0;
        for (byte i = 0; i < validRates; i++) sum += rates[i];
        bpmAvg = sum / validRates;
      }
    }
    lastBeatTime = now;
  }
  wasAbove = isAbove;

  if (millis() - lastBeatTime > 3000) bpmInst = 0;

  irBuffer[spo2Index] = irValue;
  redBuffer[spo2Index] = redValue;
  spo2Index++;

  if (spo2Index >= SPO2_SAMPLES) {
    maxim_heart_rate_and_oxygen_saturation(
      irBuffer, SPO2_SAMPLES, redBuffer,
      &spo2, &validSPO2, &dummyHR, &dummyValidHR);

    bool signalUsableForSpO2 = (irValue >= SIGNAL_LOW && irValue < SIGNAL_GOOD_MAX);

    if (validSPO2 && spo2 >= 60 && spo2 <= 100 && signalUsableForSpO2) {
      lastValidSpO2 = spo2;
      if (!spo2Ready) {
        spo2Avg = spo2;
        spo2Ready = true;
      } else {
        spo2Avg = (spo2Avg * 0.85) + (spo2 * 0.15);
      }
    }
    spo2Index = 0;
  }
}

void pagwebDebug(String str) {
  StaticJsonDocument<255> doc;
  doc["debug"] = str;
  serializeJson(doc, PagWeb);
  PagWeb.println();
}

void setup() {
  Serial.begin(115200);
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);  // UART para interfaz web
  delay(100);
  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(RUN_BUTTON, INPUT_PULLUP);
  pagwebDebug("Test Multi LM2596 Initialized...");
}

void loop() {
  // ==== Manejo del botón de arranque ====
  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();  // Limpia cualquier dato previo
    delay(100);        // Debounce

    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Arranque por botonera");
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
      PagWeb.println();
    }
  }

  // ==== Parseo de comandos Serial ====
  if (PagWeb.available()) {
    String JSON_in = PagWeb.readStringUntil('\n');

    // CANDADO 2: Validar que el JSON recibido tenga formato correcto
    DeserializationError error = deserializeJson(receiveJSON, JSON_in);
    if (error) {
      serialDebug(String("Error JSON: ") + error.c_str());
      return;
    }

    String Function = receiveJSON["Function"];
    int opc = 0;

    if (Function == "ping") opc = 1;             // {"Function":"ping"}
    else if (Function == "initSensor") opc = 2;  // {"Function":"initSensor"}
    else if (Function == "readSensor") opc = 3;  // {"Function":"readSensor"}
    else if (Function == "restart") opc = 4;     // {"Function":"restart"}

    switch (opc) {
      case 1:  // PING
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 2:
        {
          sendJSON.clear();
          if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
            sendJSON["status"] = "Error";
            sendJSON["msg"] = "MAX30102 not detected";
            sensorInitialized = false;
          } else {
            sensor.setup(50, 4, 2, 100, 411, 16384);
            sensor.setPulseAmplitudeRed(0x24);
            sensor.setPulseAmplitudeIR(0x24);
            sensor.setPulseAmplitudeGreen(0);

            sendJSON["Result"] = "OK";
            sendJSON["msg"] = "Sensor ready";
            sensorInitialized = true;
          }
          serializeJson(sendJSON, Serial);
          Serial.println();
          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 3:  // LEER MUESTRAS EN STREAM CONTINUO
        {
          sendJSON.clear();

          if (!sensorInitialized) {
            sendJSON["error"] = "Sensor no inicializado. Ejecute initSensor primero.";
            serializeJson(sendJSON, Serial);
            Serial.println();
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }

          int samplesTaken = 0;

          // BUCLE CONTINUO: Corre mientras el dedo esté en el sensor
          while (true) {
            long irValue = sensor.getIR();
            long redValue = sensor.getRed();

            // CANDADO: Verificación del dedo (Si lo quitas, se rompe el bucle)
            if (irValue < FINGER_THRESHOLD) {
              sendJSON.clear();
              sendJSON["status"] = "No finger detected";
              serializeJson(sendJSON, Serial);
              Serial.println();
              serializeJson(sendJSON, PagWeb);
              PagWeb.println();

              // Reset de variables matemáticas
              bpmInst = 0;
              bpmAvg = 0;
              validRates = 0;
              rateSpot = 0;
              lastValidSpO2 = 0;
              spo2Avg = 0;
              spo2Ready = false;
              spo2Index = 0;
              irDC = 0;
              threshold = 80;
              wasAbove = false;

              break;  // <-- Sale del bucle infinito
            }

            // Procesamos los signos vitales a la velocidad correcta
            processVitalSigns(irValue, redValue);

            // Armamos el JSON individual de esta muestra
            sendJSON.clear();
            sendJSON["status"] = "OK";
            sendJSON["IR_Data"] = irValue;
            sendJSON["RED_Data"] = redValue;
            sendJSON["sample_number"] = samplesTaken + 1;
            sendJSON["BPM_avg"] = bpmAvg;
            sendJSON["SpO2_avg"] = spo2Ready ? spo2Avg : 0.0;

            serializeJson(sendJSON, Serial);
            Serial.println();
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();

            samplesTaken++;
            delay(5);  // Retardo crítico para mantener los ~100Hz

            // CANDADO OPCIONAL: Si quieres poder detenerlo enviando otro comando Serial
            if (Serial.available()) {
              break;
            }
          }

          break;
        }

      case 4:
        {
          ESP.restart();
          break;
        }

      default:
        {
          serialDebug("Invalid Function...");
          break;
        }
    }
  }
}