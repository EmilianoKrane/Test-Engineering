// Recepción de datos por JSON en placa MCU UNIT DUAL ONE

#include <ArduinoJson.h>

#define RX 16
#define TX 17

// Creación de objeto para UNIT Dual MCU One ESP32
HardwareSerial escaner(2); 

String datos; // Variable que recibe al JSON
//char json[] = "{\"Function\":\"gps\"}";  

StaticJsonDocument<200> doc;  // Usa StaticJsonDocument para fijar tamaño

void setup() {
  Serial.begin(115200);
  //escaner.begin(115200, SERIAL_8N1, RX, TX);
  escaner.begin(115200, SERIAL_8N1, RX, TX);
}

void loop() {
   if (escaner.available()) {
    datos = escaner.readStringUntil('\n');  // Leer hasta newline (JSON en crudo)

    // Deserializa el JSON datos y guarda la información en doc 
    DeserializationError error = deserializeJson(doc, datos); 

    if (!error) {
      String Function = doc["Function"]; // Function es la variable de interés del JSON
      Serial.println("JSON: " + Function + " Datos: " + datos);
    } 
    else {
      Serial.print("Error de JSON: ");
      Serial.println(error.c_str());
    }
  }
}
