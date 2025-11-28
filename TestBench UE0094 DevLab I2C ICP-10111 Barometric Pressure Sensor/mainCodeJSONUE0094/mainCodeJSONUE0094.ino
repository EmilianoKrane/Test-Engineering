/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL UE0094 I2C Sensor Presión y Temp JSON ====
*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include "ue_i2c_icp_10111_sen.h"

// ==== Declaración de pines
#define RX2 D4  // GPIO15 como RX
#define TX2 D5  // GPIO19 como TX

#define I2C_SDA 6  // Lectura del sensor por I2C
#define I2C_SCL 7

// ==== Inicialización de objetos
HardwareSerial PagWeb(1);  // Objeto para UART2 en PULSAR como PagWeb
//TwoWire I2CBus = TwoWire(0);  // Sensor de corriente
ICP101xx sensor;

// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

void setup() {

  // Iniciar UART0 (USB) para depuración
  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");

  // Iniciar UART2 en los pines seleccionados
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("UART2 iniciado en RX=9, TX=8");

  // Inicialización del sensor
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!sensor.begin(&Wire)) {
    Serial.println("ERROR: No se pudo inicializar el sensor!");
    //while (1) delay(1000);
  }
}


void loop() {

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {

      String Function = receiveJSON["Function"];  // Function es la variable de interés del JSON

      int opc = 0;

      if (Function == "Lectura") opc = 1;

      switch (opc) {
        case 1:

          float pressure = 0;
          float temperature = 0;
          float pressAvg = 0;
          float tempAvg = 0;
          String status = "Fail";
          sendJSON.clear();  // Limpia cualquier dato previo


          // 1️⃣ Revisar si el sensor está presente en el bus
          if (!sensorDisponible()) {
            Serial.println("Sensor NO detectado.");
            sendJSON["temp"] = "N/A";
            sendJSON["press"] = "N/A";
            sendJSON["status"] = "Fail";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }

          // 2️⃣ Inicializar sensor
          if (!sensor.begin(&Wire)) {
            Serial.println("ERROR: No se pudo inicializar el sensor!");
            sendJSON["temp"] = "N/A";
            sendJSON["press"] = "N/A";
            sendJSON["status"] = "Fail";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }



          // Start a first measurement cycle, immediately returning control.
          // Optional: Measurement mode
          //    sensor.FAST: ~3ms
          //    sensor.NORMAL: ~7ms (default)
          //    sensor.ACCURATE: ~24ms
          //    sensor.VERY_ACCURATE: ~95ms
          //sensor.measureStart(sensor.VERY_ACCURATE);

          for (int i = 0; i <= 9; i++) {
            sensor.measure(sensor.VERY_ACCURATE);

            float lectTemp = sensor.getTemperatureC();
            float lectPress = sensor.getPressurePa();

            Serial.print("Pressure: ");
            Serial.print(lectPress);
            Serial.println(" Pa");

            Serial.print("Temperature: ");
            Serial.print(lectTemp);
            Serial.println(" °C");
            Serial.println("---- ---- ---- ----");

            pressure += lectPress;    // Suma iterativa de presiones
            temperature += lectTemp;  // Suma iterativa de temperatura

            delay(120);
          }

          pressAvg = pressure / 10;
          tempAvg = temperature / 10;

          if (tempAvg > 18 && tempAvg < 37 && pressAvg > 75000 && pressAvg < 80000) {
            status = "OK";
          }


          Serial.print("Temperatura promedio: ");
          String lectTemp = String(tempAvg) + " °C";
          Serial.println(lectTemp);

          Serial.print("Presión promedio: ");
          String lectPress = String(pressAvg) + " Pa";
          Serial.println(lectPress);

          sendJSON["temp"] = lectTemp;    // Envio de corriente JSON para Temp
          sendJSON["press"] = lectPress;  // Envio de corriente JSON para Press
          sendJSON["status"] = status;    // Envio de corriente JSON para Press

          serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
          PagWeb.println();

          break;
      }
    }
  }
}

bool sensorDisponible() {
  Wire.beginTransmission(0x63);
  return (Wire.endTransmission() == 0);
}
