/* 
Est firmware funciona como puente e interprete por comunicación serial entre el testbench frontend y el menu de opciones
flasheado en la memoria del ATMega328 del proyecto DIS para validar gpios y diversas funcionalidades


-> La Pulsar funciona como un puente, reportando a la PagWeb|Frontend desde su serial nativo y enlanzando con el AtMega328
por medio del UART2 declarado en los GPIOS D0 y D1
*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de GPIOS ====
#define RX2 D1        // >> GPIO D1 como RX del UART2 comunicado al ATMega328
#define TX2 D0        // >> GPIO D0 como TX del UART2 comunicado al ATMega328
#define RUN_BUTTON 4  // >> Botonera de Arranque - Pin para botón de inicio físico

// ==== Inicialización de Objetos ====
HardwareSerial DIS(1);  // Bus de UART2 para comunicación con AtMega328

// ==== Estructura de JSON ====
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;
String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

// ==== Declaración de Variables Globales ====
bool waitingResponse = false;
unsigned long sendTime = 0;
const unsigned long TIMEOUT = 3000;  // 3 segundos
String rxDIS = "";


void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas para JSON válido
  Serial.println("{\"debug\": \"" + str + "\"}");
}

void setup() {

  Serial.begin(115200);                   // >> Serial nativo para comunicación con el Frontend
  DIS.begin(9600, SERIAL_8N1, RX2, TX2);  // >> Serial 2 para comunicación con el ATMega328
  delay(100);
  serialDebug("Test DIS Ready...");



  // ==== Declaración de Entradas/Salidas ====
  pinMode(RUN_BUTTON, INPUT);

  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);
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

    Serial.println("{\"Result\":\"Fail\", \"uart\":\"Fail\", \"gpioIn\":\"Fail\", \"analog\":\"Fail\", \"sw\":\"Fail\", \"gpioOut\":\"Fail\"}");
  }
}
