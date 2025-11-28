/* 
===== CÓDIGO CONTROL MOTOR JSON ====
*/

#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de pines
#define RX2 D4
#define TX2 D5

#define DIR D1
#define PUL D0
#define highStop 4
#define lowStop 5

HardwareSerial PagWeb(1);

String JSON_entrada;
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

unsigned int period = 0;

enum Estado { SUBIENDO,
              BAJANDO };
Estado estado = SUBIENDO;


void setup() {

  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");

  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);

  pinMode(DIR, OUTPUT);
  pinMode(PUL, OUTPUT);

  pinMode(highStop, INPUT);
  pinMode(lowStop, INPUT);
}

void loop() {

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {

      String Function = receiveJSON["Function"];
      String Velocidad = receiveJSON["Vel"];

      if (Velocidad == "alta") {
        period = 450;
      } else {
        period = 1200;
      }

      int opc = 0;
      if (Function == "Demo") opc = 1;
      else if (Function == "Up") opc = 2;
      else if (Function == "Down") opc = 3;
      else if (Function == "stepUp") opc = 4;
      else if (Function == "stepDown") opc = 5;

      switch (opc) {
        case 1:
          demoDosVueltas();
          break;

        case 2:
          sendJSON.clear();
          sendJSON["Up"] = "OK";
          serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
          PagWeb.println();

          sendJSON.clear();
          subirHastaSensor();
          serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
          PagWeb.println();
          break;

        case 3:
          sendJSON.clear();
          sendJSON["Down"] = "OK";
          serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
          PagWeb.println();

          sendJSON.clear();
          bajarHastaSensor();
          serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
          PagWeb.println();
          break;

        case 4:
          sendJSON.clear();
          digitalWrite(DIR, HIGH);
          for (int i = 0; i < 50; i++) {
            if (digitalRead(highStop) == 0) {
              sendJSON["Up"] = "Stop";
              break;
            }  // safety stop
            sendJSON["Up"] = "OK";
            stepPulse();
          }
          serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
          PagWeb.println();
          break;

        case 5:
          sendJSON.clear();
          digitalWrite(DIR, LOW);
          for (int i = 0; i < 50; i++) {
            if (digitalRead(lowStop) == 0) {
              sendJSON["Down"] = "Stop";
              break;
            }  // safety stop
            sendJSON["Down"] = "OK";
            stepPulse();
          }
          serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
          PagWeb.println();
          break;
      }
    }
  }
}





// ==== FUNCIONES DE TRABAJO ====

void stepPulse() {
  digitalWrite(PUL, HIGH);
  delayMicroseconds(period);
  digitalWrite(PUL, LOW);
  delayMicroseconds(period);
}

void subirHastaSensor() {
  digitalWrite(DIR, HIGH);
  while (digitalRead(highStop) == 1) {  // mientras NO haya tocado
    stepPulse();
    if (digitalRead(highStop) == 0) {
      sendJSON["Up"] = "Stop";
    }
  }
}

void bajarHastaSensor() {
  digitalWrite(DIR, LOW);
  while (digitalRead(lowStop) == 1) {  // mientras NO haya tocado
    stepPulse();
    if (digitalRead(lowStop) == 0) {
      sendJSON["Down"] = "Stop";
    }
  }
}

void demoDosVueltas() {

  int ciclos = 0;

  while (ciclos < 2) {

    // SUBIR → hasta tocar sensor superior
    digitalWrite(DIR, HIGH);
    while (digitalRead(highStop) == 1) {
      stepPulse();
    }

    delay(200);  // Pequeña pausa
    // Cambiar a bajar

    // BAJAR → hasta tocar sensor inferior
    digitalWrite(DIR, LOW);
    while (digitalRead(lowStop) == 1) {
      stepPulse();
    }

    delay(200);  // Pequeña pausa

    ciclos++;  // Se completó un ciclo arriba-abajo
  }

  Serial.println("Demo terminada (2 ciclos)");
}