/*
=== INTEGRACIÓN ENTRE PAGWEB <-> PULSAR <->  BASE DE DATOS ===
*/


// --- BIBLIOTECAS --- 
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Pines para UART2 (puedes reasignarlos)
#define RX2 D4    // GPIO9 como RX
#define TX2 D5    // GPIO8 como TX

// Pines para comunicación I2C con el sensor de corriente
#define I2C_SDA 6
#define I2C_SCL 7

// Pines de botones
#define BTN1 20 // Botón de cortocircuito
#define BTN2 21 // Botón de polaridad

// Pines de relés (activo en LOW)
#define RELAY1 4    // Relevadores de polaridad
#define RELAY2 22

#define RELAY3 1    // Relevadores de cortocircuito
#define RELAY4 3

// Declaración de EndPoint Data Base 
const char* ssid = "UNIT_ELECTRONICS_Wi-Fi5";
const char* password = "UNIT3l3ctr0n1cs";
const char* URLTestBench = "http://192.168.15.6:6032/BoardTesting/setNewTestBench";
const char* URLNewTest = "http://192.168.15.6:6032/BoardTesting/setNewTest";

// Comunicación UART Creación Objeto 
HardwareSerial PagWeb(1); // Crear objeto para UART2 en PULSAR como PagWeb
 
// Comunicación I2C
TwoWire I2CBus = TwoWire(0);
Adafruit_INA219 ina219;

const float shuntOffset_mV = 0.0;  // Offset en vacío para lectura inicial
const float R_SHUNT = 0.05;  // Resistencia Shunt = 50 mΩ
float corrienteSensor = 0; // Variable de lectura de corriente con el sensor

// Inicialización de botones
bool estadoBtn1 = false;
bool estadoBtn2 = false;

// JSON para recibir datos de la PagWeb
String JSON; // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> datosJSON;  // Usa StaticJsonDocument para fijar tamaño

// JSON para enviar datos a la PagWeb
String JSONCorriente;
StaticJsonDocument<200> enviarJSON;

// JSON para NewTestBench
String jsonSendTestBench;
StaticJsonDocument<200> jsonTestBench;

// JSON para NewTest
String jsonSendTest;
StaticJsonDocument<200> jsonNewTest;


void setup() {

  // Iniciar UART0 (USB) para depuración
  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");
                      
  // Iniciar UART2 en los pines seleccionados
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("UART2 iniciado en RX=9, TX=8");

  // Iniciar WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado a WiFi");

  // Iniciar comunicación I2C
  I2CBus.begin(I2C_SDA, I2C_SCL);
  if (!ina219.begin(&I2CBus)) {
    Serial.println("No se encontró el chip INA219");
    while (1) { delay(10); }
  }

  // Configurar botones con resistencia pull-up interna
  pinMode(BTN1, INPUT_PULLUP); // Cortocircuito
  pinMode(BTN2, INPUT_PULLUP);

  // Configurar pines de relés como salida
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  // Apagar todos los relés al inicio (HIGH = apagado)
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);

  Serial.println("Midiendo voltaje y corriente con el INA219 ...");
}



void loop() {

  //if (PagWeb.available() || digitalRead(BTN1) == LOW  || digitalRead(BTN2) == LOW){
  if (PagWeb.available()){

    //  Accionamiento desde PagWeb
    if (PagWeb.available() && digitalRead(BTN1) == HIGH && digitalRead(BTN2) == HIGH){

      JSON = PagWeb.readStringUntil('\n');  // Leer hasta newline (JSON en crudo)

      // Deserializa el JSON y guarda la información en datosJSON 
      DeserializationError error = deserializeJson(datosJSON, JSON); 

      if (!error) { // Si no hay error de comunicación, EJECUTA

        String Function = datosJSON["Function"]; // Function es la variable de interés del JSON
        String ID = datosJSON["ID"]; // ID leído por el sensor enviado por JSON
        String Estado = datosJSON["Estado"]; // Estado final de la prueba por JSON

        //Serial.println("JSON: " + ID + " Datos: " + JSON);
        Serial.println("JSON: " + Function + " Datos: " + JSON);

        int opc = 0; // Variable de switcheo

        if (Function == "Cortocircuito")        opc = 1;
        else if (Function == "Polaridad")       opc = 2;
        else if (Function == "Lectura Nom")     opc = 3;
        else if (Function == "Lectura Break")   opc = 4;

        else if (Function == "TestCorto")       opc = 5;  // NewTest
        else if (Function == "TestPolaridad")   opc = 6;  // NewTest
        else if (Function == "TestCarga")       opc = 7;  // NewTest
        
        else if (Function == "Enviar ID")       opc = 10; // SetNewTestBench


        switch (opc){
          
          // Prueba de cortocircuito
          case 1: 
            Serial.println("Ejecución de prueba de Cortocircuito"); 
            enviarJSON.clear();   // Limpia cualquier dato previo

            // Accionamiento de relevadores
            digitalWrite(RELAY1, LOW);    // Activo
            digitalWrite(RELAY2, LOW);    // Activo
            
            delay(6); // Tiempo de anti rebote RECOMENDABLE 6
            corrienteSensor = medirCorriente();
            delay(30);

            digitalWrite(RELAY1, HIGH);    // Apagado
            digitalWrite(RELAY2, HIGH);    // Apagado
            delay(25);

            Serial.print("Corriente en Corto: ");
            Serial.print(corrienteSensor, 3);
            Serial.println(" A");

            JSONCorriente = String(corrienteSensor, 3) + " A"; // Empaquetamiento
            enviarJSON["LecturaCorto"] = JSONCorriente; // Envio de corriente JSON para corto
            serializeJson(enviarJSON, PagWeb); // Envío de datos por JSON a la PagWeb
            PagWeb.println();   // Salto de línea para delimitar

            Serial.println("Fin de la prueba de corto");
            break; 

          // Prueba de Polaridad 
          case 2:
            Serial.println("Ejecución de prueba de Polaridad");   
            enviarJSON.clear();   // Limpia cualquier dato previo

            // Accionamiento de relevadores
            digitalWrite(RELAY3, LOW);    // Activo
            digitalWrite(RELAY4, LOW);    // Activo

            delay(25); // Tiempo de anti rebote
            corrienteSensor = medirCorriente();
            delay(10); // Tiempo de anti rebote

            digitalWrite(RELAY3, HIGH);    // Apagado
            digitalWrite(RELAY4, HIGH);    // Apagado

            Serial.print("Corriente en inv de Polaridad: ");
            Serial.print(corrienteSensor, 3);
            Serial.println(" A");

            JSONCorriente = String(corrienteSensor, 3) + " A"; // Empaquetamiento
            enviarJSON["LecturaPolaridad"] = JSONCorriente; // Envio de corriente JSON para polaridad
            serializeJson(enviarJSON, PagWeb); // Envío de datos por JSON a la PagWeb
            PagWeb.println();   // Salto de línea para delimitar

            Serial.println("Fin de la prueba");
            break; 

          // Lectura nominal de corriente
          case 3:
            Serial.println("Ejecución de Lectura Nominal");   
            enviarJSON.clear();   // Limpia cualquier dato previo

            corrienteSensor = medirCorriente();
            Serial.print("Corriente Nominal: ");
            Serial.print(corrienteSensor, 3);
            Serial.println(" A");

            JSONCorriente = String(corrienteSensor, 3) + " A"; // Empaquetamiento
            enviarJSON["Lectura"] = JSONCorriente; // Envio de corriente JSON

            if (corrienteSensor > 2.9){
              enviarJSON["Result"] = "OK"; // Clave JSON OK
            }

            serializeJson(enviarJSON, PagWeb); // Envío de datos por JSON a la PagWeb
            PagWeb.println();   // Salto de línea para delimitar
            break; 

          // Lectura en break 
          case 4:
            Serial.println("Ejecución de Lectura break");   
            enviarJSON.clear();   // Limpia cualquier dato previo

            corrienteSensor = medirCorriente();
            Serial.print("Corriente Nominal: ");
            Serial.print(corrienteSensor, 3);
            Serial.println(" A");

            JSONCorriente = String(corrienteSensor, 3) + " A"; // Empaquetamiento
            enviarJSON["Lectura"] = JSONCorriente; // Envio de corriente JSON

            if (corrienteSensor < 0.1){
              enviarJSON["Result"] = "OK"; // Clave JSON OK
            }

            serializeJson(enviarJSON, PagWeb); // Envío de datos por JSON a la PagWeb
            PagWeb.println();   // Salto de línea para delimitar
            break; 

          // Envío de datos Test de cortocircuito
          case 5: 
            Serial.println("JSON: " + Estado + " Datos: " + JSON);

            // Valores fijos temporales
            jsonNewTest["id_setPruebas"] = "441";
            jsonNewTest["id_tipo_prueba"] = 15; // id de Cortocircuito 

            if(Estado == "OK"){
              jsonNewTest["id_status_prueba"] = 1; // Prueba OK
            }
            else if(Estado == "FALLA"){
              jsonNewTest["id_status_prueba"] = 2; // Prueba ERROR
            }

            jsonNewTest["comentarios"] = "Prueba 4 de Corto";
            jsonNewTest["parametro_1"] = "0";
            jsonNewTest["parametro_2"] = "1";
            jsonNewTest["parametro_3"] = "2";

            serializeJson(jsonNewTest, jsonSendTest);

            sendPOST(URLNewTest, jsonSendTest); // Envío de NewTest a DataB
            Serial.println("Se envío nuevo Test de Cortocircuito con Estado");
            break; 

          // Envío de datos Test de polaridad
          case 6: 
            Serial.println("JSON: " + Estado + " Datos: " + JSON);

            // Valores fijos temporales
            jsonNewTest["id_setPruebas"] = "441";
            jsonNewTest["id_tipo_prueba"] = 16; // id de Polaridad

            if(Estado == "OK"){
              jsonNewTest["id_status_prueba"] = 1; // Prueba OK
            }
            else if(Estado == "FALLA"){
              jsonNewTest["id_status_prueba"] = 2; // Prueba ERROR
            }

            jsonNewTest["comentarios"] = "Prueba 4 de Polaridad";
            jsonNewTest["parametro_1"] = "0";
            jsonNewTest["parametro_2"] = "1";
            jsonNewTest["parametro_3"] = "2";

            serializeJson(jsonNewTest, jsonSendTest);

            sendPOST(URLNewTest, jsonSendTest); // Envío de NewTest a DataB
            Serial.println("Se envío nuevo Test de Inv Polaridad con Estado");
            break; 

          // Envío de datos Test de carga variable
          case 7: 
            Serial.println("JSON: " + Estado + " Datos: " + JSON);

            // Valores fijos temporales
            jsonNewTest["id_setPruebas"] = "441";
            jsonNewTest["id_tipo_prueba"] = 18; // id de Carga Variable

            if(Estado == "OK"){
              jsonNewTest["id_status_prueba"] = 1; // Prueba OK
            }
            else if(Estado == "FALLA"){
              jsonNewTest["id_status_prueba"] = 2; // Prueba ERROR
            }

            jsonNewTest["comentarios"] = "Prueba 4 de Carga V";
            jsonNewTest["parametro_1"] = "0";
            jsonNewTest["parametro_2"] = "1";
            jsonNewTest["parametro_3"] = "2";

            serializeJson(jsonNewTest, jsonSendTest);

            sendPOST(URLNewTest, jsonSendTest); // Envío de NewTest a DataB
            Serial.println("Se envío nuevo Test de Carga Variable con Estado");
            break; 

          // Envío de ID a la Base de Datos 
          case 10: 
            Serial.println("JSON: " + ID + " Datos: " + JSON);

            jsonTestBench["id_proyecto"] = "84"; // ID Asignada por la DataBase
            jsonTestBench["id_mac"] = "";
            jsonTestBench["uid"] = "";
            jsonTestBench["id_numero_serie"] = ID; // Incluye ID en JSON
            jsonTestBench["id_tecnico"] = 0;
            jsonTestBench["comentarios_generales"] = "Prueba 7 envío de ID";

            serializeJson(jsonTestBench, jsonSendTestBench);

            sendPOST(URLTestBench, jsonSendTestBench); // Envío de NewTestBench a DataB
            Serial.println("Se envío nuevo TestBench con ID");
            break; 
        }
      } 

      else {
        Serial.print("Error de JSON: ");
        Serial.println(error.c_str());
      }

    }

    // Accionamiento por botón de Cortocircuito
    else if(digitalRead(BTN1) == LOW && digitalRead(BTN2) == HIGH){

        Serial.println("Ejecución de prueba de Cortocircuito Manual"); 

        // Accionamiento de relevadores
        digitalWrite(RELAY1, LOW);    // Activo
        digitalWrite(RELAY2, LOW);    // Activo
        
        delay(5); // Tiempo de anti rebote
        corrienteSensor = medirCorriente();

        digitalWrite(RELAY1, HIGH);    // Apagado
        digitalWrite(RELAY2, HIGH);    // Apagado
        delay(25);

        PagWeb.write("2"); // Caracter para indicador amarillo en PagWeb
        delay(2000); // Tiempo de espera para mostrar el amarillo

        Serial.print("Corriente en Corto: ");
        Serial.print(corrienteSensor, 3);
        Serial.println(" A");

        PagWeb.write("1"); // Caracter para indicador verde en PagWeb
        delay(1000);
        PagWeb.write("0"); // Caracter para indicador rojo en PagWeb
        Serial.println("Fin de la prueba");
    }

    // Accionamiento por botón de Polaridad
    else if(digitalRead(BTN2) == LOW && digitalRead(BTN1) == HIGH){

        Serial.println("Ejecución de prueba de Polaridad Manual"); 

        // Accionamiento de relevadores
        digitalWrite(RELAY3, LOW);    // Activo
        digitalWrite(RELAY4, LOW);    // Activo

        delay(25); // Tiempo de anti rebote
        corrienteSensor = medirCorriente();

        digitalWrite(RELAY3, HIGH);    // Apagado
        digitalWrite(RELAY4, HIGH);    // Apagado
        delay(25);

        PagWeb.write("5"); // Caracter para indicador amarillo en PagWeb
        delay(2000); // Tiempo de espera para mostrar el amarillo

        Serial.print("Corriente en Corto: ");
        Serial.print(corrienteSensor, 3);
        Serial.println(" A");

        PagWeb.write("4"); // Caracter para indicador verde en PagWeb
        delay(1000);
        PagWeb.write("3"); // Caracter para indicador rojo en PagWeb 
        Serial.println("Fin de la prueba");
    }

    // Falso contacto
    else{
        digitalWrite(RELAY1, HIGH);    // Apagado
        digitalWrite(RELAY2, HIGH);    // Apagado
        digitalWrite(RELAY3, HIGH);    // Apagado
        digitalWrite(RELAY4, HIGH);    // Apagado
    }



  }


} // void loop()
                                                                                                                                    

// ## DECLARACIÓN DE FUNCIONES ##

// ==== Medición de Corriente INA219 ====
float medirCorriente(){

  float shunt_mV = ina219.getShuntVoltage_mV();
  float bus_V = ina219.getBusVoltage_V();

  shunt_mV -= shuntOffset_mV;  
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT; // Corriente de interés
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;

  return current_A;
}

// ==== Envío de información a la Base de Datos ====
int sendPOST(const char* URL, String JSON) {
    int id_testBench = -1; // valor por defecto si falla

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(URL);
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(JSON);

        if (httpResponseCode > 0) {
            Serial.println("POST Enviado correctamente!");
            String response = http.getString();
            Serial.println(response);

            StaticJsonDocument<200> doc;
            deserializeJson(doc, response);

            id_testBench = doc["data"][0]["id_testBench"];
            Serial.print("El id es: ");
            Serial.println(id_testBench);
        } else {
            Serial.print("Error al enviar el POST: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    }

    return id_testBench; 
}


