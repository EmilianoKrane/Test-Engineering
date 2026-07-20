/*

 */

#include <Wire.h>
#include "DevLab_MAX30102.h"
#include "spo2_algorithm.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ==== DECLARACIÓN DE PINES ====
#define RUN_BUTTON 4  // >> Botonera de Arranque - Pin para botón de inicio físico
#define SDA_PIN 6     // >> SDA para I2C con el esclavo - Línea de datos I2C
#define SCL_PIN 7     // >> SCL para I2C con el esclavo - Línea de reloj I2C



// ==== CREACIÓN DE OBJETOS ====
MAX30105 sensor;
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos
StaticJsonDocument<1024> sendJSON;     ///< Documento JSON para armar respuestas


// ==== DECLARACIÓN DE VARIABLES GLOBALES ====
#define SPO2_SAMPLES 100
const byte RATE_SIZE = 8;  // BPM measurement
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


// ==== FUNCTIONES DE UTILIDAD ====
void serialDebug(String str) {
  StaticJsonDocument<255> doc;
  doc["debug"] = str;
  serializeJson(doc, Serial);
  Serial.println();
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!sensor.begin(Wire, I2C_SPEED_FAST)) serialDebug("MAX30102 not detected...");
  else serialDebug("MAX30102 initialized...");

  // ==== DECLARACIÓN DE PINES ====
  pinMode(RUN_BUTTON, INPUT_PULLUP);


  // ==== CONFIGURACIÓN DEL SENSOR ====
  sensor.setup(
    50,    // LED brightness
    4,     // Sample average
    2,     // RED + IR
    100,   // Sample rate: 100 Hz
    411,   // Pulse width
    16384  // ADC range
  );
  sensor.setPulseAmplitudeRed(0x24);
  sensor.setPulseAmplitudeIR(0x24);
  sensor.setPulseAmplitudeGreen(0);
}

void loop() {

  // ==== Manejo del botón de arranque ====
  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();  // Limpia cualquier dato previo
    delay(100);        // Debounce

    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Arranque por botonera");
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, Serial);  // Envío de datos por JSON a la PagWeb
      Serial.println();
    }
  }

  if (Serial.available()) {
    String JSON_in = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_in);

    String Function = receiveJSON["Function"];

    int opc = 0;
    if (Function == "ping") opc = 1;  // {"Function":"ping"}


    switch (opc) {
      case 1:
        {
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      default: serialDebug("Invalid option..."); break;
    }
  }
}






/*


  long irValue = sensor.getIR();
  long redValue = sensor.getRed();

  if (irValue < FINGER_THRESHOLD) {
    Serial.println(F("No finger detected"));

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

    delay(300);
    return;
  }

  String signalQuality;

  if (irValue < SIGNAL_LOW) {
    signalQuality = "Low";
  } else if (irValue < SIGNAL_GOOD_MAX) {
    signalQuality = "Good";
  } else {
    signalQuality = "Saturated";
  }

  bool signalUsableForSpO2 = (irValue >= SIGNAL_LOW && irValue < SIGNAL_GOOD_MAX);

  if (irDC == 0) {
    irDC = irValue;
  }

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

        if (validRates < RATE_SIZE) {
          validRates++;
        }

        float sum = 0;
        for (byte i = 0; i < validRates; i++) {
          sum += rates[i];
        }

        bpmAvg = sum / validRates;
      }
    }

    lastBeatTime = now;
  }

  wasAbove = isAbove;

  if (millis() - lastBeatTime > 3000) {
    bpmInst = 0;
  }

  irBuffer[spo2Index] = irValue;
  redBuffer[spo2Index] = redValue;
  spo2Index++;

  if (spo2Index >= SPO2_SAMPLES) {
    maxim_heart_rate_and_oxygen_saturation(
      irBuffer,
      SPO2_SAMPLES,
      redBuffer,
      &spo2,
      &validSPO2,
      &dummyHR,
      &dummyValidHR);

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

  String spo2Status = "OK";

  if (spo2Ready && spo2Avg < 90) {
    spo2Status = "LOW/Check";
  } else if (!signalUsableForSpO2) {
    spo2Status = "Signal not ideal";
  }

  char line[220];

  if (spo2Ready) {
    snprintf(line, sizeof(line),
             "IR:%6ld | RED:%6ld | AC:%8.1f | BPM inst:%6.1f | BPM avg:%6.1f | SpO2:%5.1f%% | Status:%-16s | Signal:%s",
             irValue,
             redValue,
             pulseSignal,
             bpmInst,
             bpmAvg,
             spo2Avg,
             spo2Status.c_str(),
             signalQuality.c_str());
  } else {
    snprintf(line, sizeof(line),
             "IR:%6ld | RED:%6ld | AC:%8.1f | BPM inst:%6.1f | BPM avg:%6.1f | SpO2:  ---  | Status:%-16s | Signal:%s",
             irValue,
             redValue,
             pulseSignal,
             bpmInst,
             bpmAvg,
             spo2Status.c_str(),
             signalQuality.c_str());
  }

  Serial.println(line);
  delay(200);




*/