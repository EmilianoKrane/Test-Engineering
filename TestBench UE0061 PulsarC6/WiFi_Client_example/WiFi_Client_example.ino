#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *ssid = "ESP32-Master-Red";
const char *password = "123456789";

// La IP por defecto del SoftAP del Master es 192.168.4.1
const char *serverUrl = "http://192.168.4.1/ping";

void setup() {
  Serial.begin(115200);
  
  // Conectarse a la red del Master
  WiFi.begin(ssid, password);
  Serial.print("Conectando al Master");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n¡Conectado a la red del Master!");
}

void loop() {
  // Asegurarnos de que seguimos conectados antes de enviar datos
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    
    // Especificar que el contenido que enviamos es un JSON
    http.addHeader("Content-Type", "application/json");

    // 1. Crear el JSON de envío (Ping)
    JsonDocument docReq;
    docReq["dispositivo"] = "Esclavo_B";
    docReq["mensaje"] = "Ping para validar conexion";
    
    String jsonRequest;
    serializeJson(docReq, jsonRequest);

    Serial.println("\nEnviando: " + jsonRequest);
    
    // 2. Enviar la petición POST y guardar el código de respuesta HTTP
    int httpResponseCode = http.POST(jsonRequest);

    // 3. Procesar la respuesta del Master
    if (httpResponseCode > 0) {
      String payload = http.getString(); // Extraer el JSON del Master
      
      JsonDocument docRes;
      DeserializationError error = deserializeJson(docRes, payload);
      
      if (!error) {
        String respuestaMaster = docRes["respuesta"];
        long uptimeMaster = docRes["master_uptime_ms"];
        
        Serial.printf("Master respondió (HTTP %d):\n", httpResponseCode);
        Serial.printf(" - Mensaje: %s\n", respuestaMaster.c_str());
        Serial.printf(" - Uptime Master: %ld ms\n", uptimeMaster);
      } else {
        Serial.println("Error al parsear el JSON recibido del Master.");
      }
    } else {
      Serial.printf("Error en la petición HTTP. Código: %d\n", httpResponseCode);
    }
    
    // Liberar recursos de la conexión
    http.end(); 
  }
  
  // Esperar 5 segundos antes de realizar el siguiente ping
  delay(5000); 
}