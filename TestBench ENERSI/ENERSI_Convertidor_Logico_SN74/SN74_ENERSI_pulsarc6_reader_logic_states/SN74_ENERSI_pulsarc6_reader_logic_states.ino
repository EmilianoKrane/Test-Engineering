/*
Este firmware se carga en la pulsar c6 para leer la salida del convertidor logico sn74
por medio de un divisor de tensión, el umbral de valores validos se ajusta segun la salida 
obtenida bajando 3.3 a la mitad como estado alto.
*/

#include <ArduinoJson.h>
#include <HardwareSerial.h>

#define INPUT_A0 0
#define INPUT_A1 1
#define INPUT_A2 2
#define INPUT_A3 3

// ==== Declaración de variables
String JSON_entrada;
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

// Resolución del ADC y voltaje de referencia del ESP32-C6
const float ADC_RESOLUTION = 4095.0;
const float V_REF = 3.3;

void setup() {
  Serial.begin(115200);
}

// Función auxiliar para generar el JSON basado en el voltaje
void getStatusJSON(String pinName, float voltage) {

  String status = (voltage >= 1.2 && voltage <= 1.7) ? "OK" : "ERROR";

  StaticJsonDocument<256> doc;
  doc.clear();
  doc["pin"] = pinName;
  doc["voltage"] = String(voltage, 2);
  doc["status"] = status;
  serializeJson(doc, Serial);
  Serial.println();
}

void loop() {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (error) {
      sendJSON.clear();
      sendJSON["status"] = "FAIL";
      sendJSON["error"] = String("Invalid JSON: ") + error.c_str();
      serializeJson(sendJSON, Serial);
      Serial.println();
    } else {

      String Function = receiveJSON["Function"];
      int opc = 0;
      if (Function == "ping") opc = 1;     // {"Function":"ping"}
      else if (Function == "tx") opc = 2;  // {"Function":"tx"}

      switch (opc) {

        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serialiazeJson(sendJSON, Serial);
            Serial.pritnln();
            1 break;
          }
      }
    }
  }

  delay(500);
}



/*

  // 1. Leer los pines analógicos
  int raw_a0 = analogRead(INPUT_A0);
  int raw_a1 = analogRead(INPUT_A1);
  int raw_a2 = analogRead(INPUT_A2);
  int raw_a3 = analogRead(INPUT_A3);

  // 2. Convertir las lecturas a voltaje
  float voltage_a0 = analogReadMilliVolts(INPUT_A0) / 1000.0;
  float voltage_a1 = (raw_a1 / ADC_RESOLUTION) * V_REF;
  float voltage_a2 = (raw_a2 / ADC_RESOLUTION) * V_REF;
  float voltage_a3 = (raw_a3 / ADC_RESOLUTION) * V_REF;

  // 3. Evaluar el rango e imprimir el JSON
  getStatusJSON("A0", voltage_a0);
  getStatusJSON("A1", voltage_a1);
  getStatusJSON("A2", voltage_a2);
  getStatusJSON("A3", voltage_a3);


*/
