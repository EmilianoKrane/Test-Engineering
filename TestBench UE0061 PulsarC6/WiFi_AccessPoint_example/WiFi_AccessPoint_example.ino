#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char *ssid = "ESP32-Master-Red";
const char *password = "123456789";

// Iniciamos el servidor en el puerto 80
WebServer server(80);

void handlePing() {
  // 1. Verificamos que el Esclavo haya enviado información en el cuerpo (body)
  if (server.hasArg("plain") == false) {
    server.send(400, "text/plain", "No se recibió JSON");
    return;
  }

  // 2. Leemos y parseamos el JSON recibido
  String body = server.arg("plain");
  JsonDocument docReq;
  DeserializationError error = deserializeJson(docReq, body);

  if (error) {
    server.send(400, "text/plain", "JSON Inválido");
    return;
  }

  String mensajeRecibido = docReq["mensaje"];
  Serial.println("Recibido del Esclavo: " + mensajeRecibido);

  // 3. Armamos el JSON de respuesta (Pong)
  JsonDocument docRes;
  docRes["estatus"] = "Conexión Validada";
  docRes["respuesta"] = "Pong desde el Master";
  docRes["master_uptime_ms"] = millis();

  String jsonResponse;
  serializeJson(docRes, jsonResponse);

  // 4. Enviamos el JSON de vuelta al Esclavo con código HTTP 200 (OK)
  server.send(200, "application/json", jsonResponse);
}

void setup() {
  Serial.begin(115200);
  
  // Configurar e iniciar el Punto de Acceso
  WiFi.softAP(ssid, password);
  Serial.print("Master AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Indicar al servidor qué función ejecutar cuando reciba un POST en "/ping"
  server.on("/ping", HTTP_POST, handlePing);
  server.begin();
  
  Serial.println("Servidor HTTP iniciado. Esperando pings...");
}

void loop() {
  // Mantiene al servidor escuchando constantemente
  server.handleClient();
}