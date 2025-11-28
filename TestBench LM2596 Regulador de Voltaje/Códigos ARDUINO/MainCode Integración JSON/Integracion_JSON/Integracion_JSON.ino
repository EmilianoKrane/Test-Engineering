// Código Final Integrado para LM2596  entre PagWeb y Pulsar ESP32-C6 LM2596

// --- BIBLIOTECAS --- 
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
//#include <WiFi.h>
//#include <HTTPClient.h>

// Pines para UART2 (puedes reasignarlos)
#define RX2 D4    // GPIO9 como RX
#define TX2 D5    // GPIO8 como TX

// Pines para comunicación I2C con el sensor de corriente
#define I2C_SDA 6
#define I2C_SCL 7

// Relevadores de Cortocircuito
#define RELAY1 14
#define RELAY2 0

// Relevadores de Accionamiento de Fuente
#define RELAYA 8 //Problema no agarra 
#define RELAYB 9



// Comunicación UART Creación Objeto 
HardwareSerial PagWeb(1); // Crear objeto para UART2 en PULSAR como PagWeb
 
// Comunicación I2C
TwoWire I2CBus = TwoWire(0);
Adafruit_INA219 ina219;

const float shuntOffset_mV = 0.0;  // Offset en vacío para lectura inicial
const float R_SHUNT = 0.05;  // Resistencia Shunt = 50 mΩ
float corrienteSensor = 0; // Variable de lectura de corriente con el sensor
float voltajeSensor = 0; // Variable de lectura de voltaje con el sensor

// JSON para recibir datos
String JSON; // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> datosJSON;  // Usa StaticJsonDocument para fijar tamaño

// JSON para enviar datos
String JSONCorriente;
StaticJsonDocument<200> enviarJSON;


void setup() {

  // Iniciar UART0 (USB) para depuración
  Serial.begin(115200);
  //while(!Serial) { delay(10); }  // <= agrega esto
  Serial.println("UART0 listo (USB)");
                      
  // Iniciar UART2 en los pines seleccionados
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("UART2 iniciado en RX=9, TX=8");


  // Iniciar comunicación I2C
  I2CBus.begin(I2C_SDA, I2C_SCL);

  if (!ina219.begin(&I2CBus)) {
    Serial.println("No se encontró el chip INA219");
    while (1) { delay(10); }
  }

  // Configurar pines de relés como salida
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);

  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);

  // Apagar todos los relés al inicio (HIGH = apagado)
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);

  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);

  Serial.println("Midiendo voltaje y corriente con el INA219 ...");
}



void loop() {


  if (PagWeb.available()){

    JSON = PagWeb.readStringUntil('\n');  // Leer hasta newline (JSON en crudo)

    // Deserializa el JSON y guarda la información en datosJSON 
    DeserializationError error = deserializeJson(datosJSON, JSON); 

    //char dato = PagWeb.read(); // Caracter de entrada para seleccionar prueba

    if (!error) { // Si no hay error de comunicación, EJECUTA

      String Function = datosJSON["Function"]; // Function es la variable de interés del JSON

      int opc = 0; // Variable de switcheo

      // Llaves JSON de Ejecución de pruebas
      if (Function == "Cortocircuito")        opc = 1;
      else if (Function == "Lectura Nom")     opc = 2;

      switch (opc){
        
        // Prueba de cortocircuito
        case 1: 
          Serial.println("Ejecución de prueba de Cortocircuito"); 
          enviarJSON.clear();   // Limpia cualquier dato previo

          // Accionamiento de relevadores
          digitalWrite(RELAY1, LOW);    // Activo
          digitalWrite(RELAY2, LOW);    // Activo
          delay(1000);

          corrienteSensor = medirValores();
          delay(500);

          digitalWrite(RELAY1, HIGH);    // Apagado
          digitalWrite(RELAY2, HIGH);    // Apagado

          Serial.print("Corriente en Corto: ");
          Serial.print(corrienteSensor, 3);
          Serial.println(" A");

          if (fabs(corrienteSensor) < 0.8){
            enviarJSON["Result"] = "OK"; // Clave JSON OK
          }

          JSONCorriente = String(corrienteSensor, 3) + " A"; // Empaquetamiento
          enviarJSON["LecturaCorto"] = JSONCorriente; // Envio de corriente JSON para corto
          serializeJson(enviarJSON, PagWeb); // Envío de datos por JSON a la PagWeb
          PagWeb.println();   // Salto de línea para delimitar

          Serial.println("Fin de la prueba de corto");
          break; 

        // Lectura nominal de corriente
        case 2:
          Serial.println("Ejecución de Lectura Nominal");   
          enviarJSON.clear();   // Limpia cualquier dato previo

          corrienteSensor = medirValores();

          Serial.print("Corriente Nominal: ");
          Serial.print(corrienteSensor, 3);
          Serial.println(" A");

          JSONCorriente = String(corrienteSensor, 3) + " A"; // Empaquetamiento
          enviarJSON["Lectura"] = JSONCorriente; // Envio de corriente JSON

          if (corrienteSensor > 2.5){
            enviarJSON["Result"] = "OK"; // Clave JSON OK
          }

          serializeJson(enviarJSON, PagWeb); // Envío de datos por JSON a la PagWeb
          PagWeb.println();   // Salto de línea para delimitar
          break; 

      }
    } 

    else {
      Serial.print("Error de JSON: ");
      Serial.println(error.c_str());
    }
  }


  // Falso contacto
  else{
      digitalWrite(RELAY1, HIGH);    // Apagado
      digitalWrite(RELAY2, HIGH);    // Apagado
  }
} // void loop()
                                                                                                                                    

// ## DECLARACIÓN DE FUNCIONES ##

// ==== Medición INA219 ====
float medirValores(){

  float shunt_mV = ina219.getShuntVoltage_mV();
  float bus_V = ina219.getBusVoltage_V();

  shunt_mV -= shuntOffset_mV;  
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT; // Corriente de interés
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;

  return current_A;
}

