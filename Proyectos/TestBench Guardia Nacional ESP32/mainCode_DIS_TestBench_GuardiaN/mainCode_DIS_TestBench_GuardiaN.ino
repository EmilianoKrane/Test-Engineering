/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL DIS GUARDIA NACIONAL ====
Este código funciona unicamente como un Passthrough entre la PagWeb y el arnés para DIS

-- Como conexiones, la PagWeb selecciona el COM de la Pulsar en el Test y el conector QWIIC del arnés
se conecta al bus de pines GPIO01 y GPIO02 UARTL
*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de pines
#define RX2 D1        // GPIO como RXD
#define TX2 D0        // GPIO como TXD
#define RUN_BUTTON 4  // Botón de Arranque

// ==== Inicialización de objetos
HardwareSerial DIS(1);  // Objeto para UART2 en PULSAR como PagWeb

// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

bool waitingResponse = false;
unsigned long sendTime = 0;
const unsigned long TIMEOUT = 3000;  // 3 segundos

String rxDIS = "";


void setup() {

  Serial.begin(4800);                     // Serial enlaza la PagWeb
  DIS.begin(4800, SERIAL_8N1, RX2, TX2);  // Bus de comunicación con el CH552

  pinMode(RUN_BUTTON, INPUT);
}

void loop() {

  // ---- BOTÓN ----
  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);
    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON["Run"] = "OK";
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
  }

  // ---- SERIAL → DIS ----
  if (Serial.available()) {
    char c = Serial.read();
    DIS.write(c);

    // Detectar envío del JSON específico
    rxDIS += c;
    if (rxDIS.endsWith("{\"Function\":\"testAll\"}")) {
      waitingResponse = true;
      sendTime = millis();
      rxDIS = "";  // limpiar buffer
    }

    // Evitar que crezca infinito
    if (rxDIS.length() > 64) rxDIS = "";
  }

  // ---- DIS → SERIAL (respuesta) ----
  if (DIS.available()) {
    char c = DIS.read();
    Serial.write(c);

    // Cualquier dato recibido cuenta como respuesta
    waitingResponse = false;
  }

  // ---- TIMEOUT ----
  if (waitingResponse && (millis() - sendTime >= TIMEOUT)) {
    waitingResponse = false;

    Serial.println("{\"gpioIn\":\"Fail\", \"analog\":\"Fail\", \"sw\":\"Fail\", \"gpioOut\":\"Fail\"}");
  }
}
