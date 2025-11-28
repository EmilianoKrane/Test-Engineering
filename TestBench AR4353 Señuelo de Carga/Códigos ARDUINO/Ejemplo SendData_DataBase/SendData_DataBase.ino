// Sketch para mandar datos a la base de datos 


#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "UNIT_ELECTRONICS_Wi-Fi5";
const char* password = "UNIT3l3ctr0n1cs";
const char* URLTestBench = "http://192.168.15.6:6032/BoardTesting/setNewTestBench";
const char* URLNewTest = "http://192.168.15.6:6032/BoardTesting/setNewTest";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado a WiFi");




  // JSON a enviar
  StaticJsonDocument<200> jsonTestBench;
  jsonTestBench["id_proyecto"] = "84"; // ID Asignada por la DataBase
  jsonTestBench["id_mac"] = "";
  jsonTestBench["uid"] = "";
  jsonTestBench["id_numero_serie"] = "";
  jsonTestBench["id_tecnico"] = 0;
  jsonTestBench["comentarios_generales"] = "Prueba 3 de envio de datos";


  // JSON a enviar
  StaticJsonDocument<200> jsonNewTest;
  jsonNewTest["id_setPruebas"] = "SET123";
  jsonNewTest["id_tipo_prueba"] = 8;
  jsonNewTest["id_status_prueba"] = 1;
  jsonNewTest["parametro_1"] = 3.12;
  jsonNewTest["parametro_2"] = 0.15;
  jsonNewTest["parametro_3"] = 0.468;


  String jsonSendTestBench;
  serializeJson(jsonTestBench, jsonSendTestBench);

  sendPOST(URLTestBench, jsonSendTestBench);

}

void loop() {
  // Nada en loop
}



void sendPOST(const char* URL, String JSON){

  // Enviar POST
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    http.begin(URL);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(JSON);

    if (httpResponseCode > 0) {
      Serial.println("Enviado correctamente!");
      String response = http.getString();
      Serial.println(response);
    } 
    else {
      Serial.print("Error al enviar: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }


}


