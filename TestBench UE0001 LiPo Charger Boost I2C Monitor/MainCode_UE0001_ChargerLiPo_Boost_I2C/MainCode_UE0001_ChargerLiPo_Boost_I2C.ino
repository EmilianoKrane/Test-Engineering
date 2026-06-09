/*

*/

// ====   BIBLIOTECAS ====
#include <Wide.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// ==== DECLARACIÓN DE GPIOS ==== +
#define RUN_BUTTON 4  // >> GPIO04 Arranque por Botonera en TestBench
#define SDA_PIN 6
#define SCL_PIN 7

// ==== DECLARACIÓN DE OBJETOS ====
String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;                ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas


void setup() {
  Serial.begin(115200);
}

void loop() {
}
