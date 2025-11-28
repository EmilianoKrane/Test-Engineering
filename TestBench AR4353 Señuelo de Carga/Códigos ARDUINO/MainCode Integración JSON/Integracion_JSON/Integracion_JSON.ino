// Código Final Integrado para Señuelo de carga con PagWeb y Pulsar ESP32-C6
// Integración con TestBench

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// Pines para UART2 (puedes reasignarlos)
#define RX2 D4  // GPIO15 como RX
#define TX2 D5  // GPIO19 como TX

// Relevadores de Accionamiento de Fuente
#define RELAYA 8  //Problema no agarra
#define RELAYB 9

// Relevadores de Inv Polaridad
#define RELAYIn1 21
#define RELAYIn2 18

// Comunicación UART Creación Objeto
HardwareSerial PagWeb(1);  // Crear objeto para UART2 en PULSAR como PagWeb

// JSON para recibir datos
String JSON;
StaticJsonDocument<200> datosJSON;


void setup() {
  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");

  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("UART2 iniciado en RX=9, TX=8");

  pinMode(RELAYIn1, OUTPUT);
  pinMode(RELAYIn2, OUTPUT);

  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);


  digitalWrite(RELAYIn1, HIGH);  // Estado inicial
  digitalWrite(RELAYIn2, HIGH);  // Estado inicial

  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);


  Serial.println("Sistema de control inicializado ...");
}

void loop() {
  // Revisa si llegaron datos
  if (PagWeb.available()) {
    JSON = PagWeb.readStringUntil('\n');

    DeserializationError error = deserializeJson(datosJSON, JSON);

    if (!error) {
      String Function = datosJSON["Function"];

      int opc = 0;  // Variable de switcheo
      if (Function == "Lectura Vusb") opc = 1;
      else if (Function == "Lectura V33") opc = 2;

      switch (opc) {
        case 1:
          Serial.println("Medición de Vusb");
          digitalWrite(RELAYIn1, LOW);
          digitalWrite(RELAYIn2, LOW);
          break;

        case 2:
          Serial.println("Medición de V33");
          digitalWrite(RELAYIn1, HIGH);
          digitalWrite(RELAYIn2, HIGH);
          break;
      }




    } else {
      Serial.print("Error de JSON: ");
      Serial.println(error.c_str());
    }
  }
}
