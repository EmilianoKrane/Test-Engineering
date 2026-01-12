/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL UE0094 I2C Sensor Presión y Temp JSON ====
Este código funciona unicamente como un Passthrough entre la PagWeb y el UE0090 Multiprotocol 

-- Como conexiones, la PagWeb selecciona el COM de la Pulsar en el Test y el conector QWIIC del arnés
se conecta al bus de pines GPIO01 y GPIO02 UARTL

El firmware de prueba que se carga al Multiprotocol está en la carpeta de CocketNova/IngPruebas/Serial/build
*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de pines
#define RX2 D0        // GPIO04 como RXD
#define TX2 D1        // GPIO05 como TXD
#define RUN_BUTTON 4  // Botón de Arranque
#define RELAYCom 20   // Relevador de puerto USB C

// ==== Inicialización de objetos
HardwareSerial Multiprotocol(1);  // Objeto para UART2 en PULSAR como PagWeb

// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

void setup() {

  Serial.begin(9600);                               // Serial enlaza la PagWeb
  Multiprotocol.begin(9600, SERIAL_8N1, RX2, TX2);  // Bus de comunicación con el CH552

  pinMode(RELAYCom, OUTPUT);
  pinMode(RUN_BUTTON, INPUT);
  digitalWrite(RELAYCom, HIGH);
}

void loop() {

  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);

    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON["Run"] = "OK";  // Envio de corriente JSON para corto
      serializeJson(sendJSON, Serial);           // Envío de datos por JSON a la PagWeb
      Serial.println();
    }
  }

  if (Serial.available()) {  // If anything comes in Serial (USB),

    JSON_entrada = Serial.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {

      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "Test") opc = 1;  // {"Function":"Test"}

      switch (opc) {
        case 1:
          {
            delay(50);
            Multiprotocol.write("a");  // Caracter de inicio de Test

            while (Multiprotocol.available()) {
              Serial.write(Multiprotocol.read());
            }

            break;
          }
      }
    }
  }

  if (Multiprotocol.available()) {
    Serial.write(Multiprotocol.read());
  }
}
