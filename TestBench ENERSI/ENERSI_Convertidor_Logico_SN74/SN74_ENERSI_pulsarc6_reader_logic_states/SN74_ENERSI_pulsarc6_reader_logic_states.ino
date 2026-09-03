/*
Este firmware se carga en la pulsar c6 para leer la salida del convertidor logico sn74
por medio de un divisor de tensión, el umbral de valores validos se ajusta segun la salida 
obtenida bajando 3.3 a la mitad como estado alto.
*/

#include <ArduinoJson.h>

#define INPUT_A0 0
#define INPUT_A1 1
#define INPUT_A2 2
#define INPUT_A3 3

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

  /*
    getStatusJSON("A1", voltage_a1);
  getStatusJSON("A2", voltage_a2);
  getStatusJSON("A3", voltage_a3);
  */

  delay(500);
}