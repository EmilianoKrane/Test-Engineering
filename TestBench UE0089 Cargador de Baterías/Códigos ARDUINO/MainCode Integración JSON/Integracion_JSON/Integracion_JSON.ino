// Código Final Integrado para LM2596  entre PagWeb y Pulsar ESP32-C6

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// Pines para UART2 (puedes reasignarlos)
#define RX2 D4  // GPIO15 como RX
#define TX2 D5  // GPIO19 como TX

/*
// Pines para comunicación I2C con el sensor de corriente
#define I2C_SDA 22  //6
#define I2C_SCL 23  //7

// Relevador de puerto COM
#define RELAYCom 20

// Relevadores de Accionamiento de Fuente
#define RELAYA 8
#define RELAYB 9

// Relevadores de Inv Polaridad
#define RELAY1 21
#define RELAY2 18  // Mover a G0

// Relevadores de Cortocircuito
#define RELAY3 7  // Mover a D20
#define RELAY4 2  // Mover G1
*/

// Pines para comunicación I2C con el sensor de corriente
#define I2C_SDA 6
#define I2C_SCL 7

// Relevador de puerto COM
#define RELAYCom 20

// Relevadores de Accionamiento de Fuente
#define RELAYA 8  //Problema no agarra
#define RELAYB 9

// Relevadores de Inv Polaridad
#define RELAY1 21
#define RELAY2 1

// Relevadores de Cortocircuito
#define RELAY3 14
#define RELAY4 0


// Comunicación UART Creación Objeto
HardwareSerial PagWeb(1);  // Crear objeto para UART2 en PULSAR como PagWeb

// Comunicación I2C
TwoWire I2CBus = TwoWire(0);
Adafruit_INA219 ina219;

const float shuntOffset_mV = 0.0;  // Offset en vacío para lectura inicial
const float R_SHUNT = 0.05;        // Resistencia Shunt = 50 mΩ
float corrienteSensor = 0;         // Variable de lectura de corriente con el sensor

// JSON para recibir datos
String JSON;                        // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> datosJSON;  // Usa StaticJsonDocument para fijar tamaño

// JSON para enviar datos
String JSONCorriente;
StaticJsonDocument<200> enviarJSON;

void setup() {

  // Iniciar UART0 (USB) para depuración
  Serial.begin(115200);
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
  pinMode(RELAYCom, OUTPUT);
  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  // Apagar todos los relés al inicio (HIGH = apagado)
  digitalWrite(RELAYCom, LOW);
  digitalWrite(RELAYA, HIGH);
  digitalWrite(RELAYB, HIGH);
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);

  Serial.println("Midiendo voltaje y corriente con el INA219 ...");
}


void loop() {
  //  Accionamiento desde PagWeb
  if (PagWeb.available()) {

    JSON = PagWeb.readStringUntil('\n');  // Leer hasta newline (JSON en crudo)

    // Deserializa el JSON y guarda la información en datosJSON
    DeserializationError error = deserializeJson(datosJSON, JSON);

    if (!error) {  // Si no hay error de comunicación, EJECUTA

      String Function = datosJSON["Function"];  // Function es la variable de interés del JSON

      int opc = 0;  // Variable de switcheo

      // Llaves JSON de Ejecución de pruebas
      if (Function == "Cortocircuito") opc = 1;
      else if (Function == "Polaridad") opc = 2;
      else if (Function == "Lectura Nom") opc = 3;
      else if (Function == "Lectura Break") opc = 4;
      else if (Function == "Fuente ON") opc = 5;
      else if (Function == "Fuente OFF") opc = 6;
      else if (Function == "COM ON") opc = 7;
      else if (Function == "COM OFF") opc = 8;
      else if (Function == "Carga Bat") opc = 9;

      /*
      {"Function":"Cortocircuito"}
      {"Function":"Polaridad"}

      {"Function":"Lectura Nom"}
      {"Function":"lectura Break"}

      {"Function":"Fuente ON"}
      {"Function":"Fuente OFF"}

      {"Function":"COM ON"}
      {"Function":"COM OFF"}

      {"Function":"Carga Bat"}
      */

      switch (opc) {

        // Prueba de cortocircuito
        case 1:
          Serial.println("Ejecución de prueba de Cortocircuito");
          enviarJSON.clear();  // Limpia cualquier dato previo

          float valueMax;

          // Accionamiento de relevadores
          digitalWrite(RELAY3, LOW);  // Activo
          digitalWrite(RELAY4, LOW);  // Activo

          for (int i = 0; i < 50; i++) {
            float valorActual = medirCorriente();
            Serial.println(valorActual);

            if (valorActual > valueMax) {
              valueMax = valorActual;
            }
          }

          digitalWrite(RELAY3, HIGH);  // Apagado
          digitalWrite(RELAY4, HIGH);  // Apagado

          Serial.print("Corriente en Corto: ");
          Serial.print(valueMax, 3);
          Serial.println(" A");

          if (fabs(valueMax) > 5.5) {
            enviarJSON["status"] = "OK";
          } else {
            enviarJSON["status"] = "Fail";
          }

          JSONCorriente = String(valueMax, 1) + " A";  // Empaquetamiento
          enviarJSON["meas"] = JSONCorriente;          // Envio de corriente JSON para corto
          serializeJson(enviarJSON, PagWeb);           // Envío de datos por JSON a la PagWeb
          PagWeb.println();                            // Salto de línea para delimitar

          Serial.println("Fin de la prueba de corto");
          break;

        // Prueba de Inv de Polaridad
        case 2:
          Serial.println("Ejecución de prueba de Inv Polaridad");
          enviarJSON.clear();  // Limpia cualquier dato previo

          // Accionamiento de relevadores
          digitalWrite(RELAY1, LOW);  // Activo
          digitalWrite(RELAY2, LOW);  // Activo

          delay(500);
          corrienteSensor = medirCorriente();
          delay(10);

          digitalWrite(RELAY1, HIGH);  // Apagado
          digitalWrite(RELAY2, HIGH);  // Apagado

          Serial.print("Corriente en Inv Polaridad: ");
          Serial.print(corrienteSensor, 3);
          Serial.println(" A");

          if (fabs(corrienteSensor) < 0.1) {
            enviarJSON["status"] = "OK";
          } else {
            enviarJSON["status"] = "Fail";
          }

          JSONCorriente = String(corrienteSensor, 1) + " A";  // Empaquetamiento
          enviarJSON["meas"] = JSONCorriente;                 // Envio de corriente JSON para corto
          serializeJson(enviarJSON, PagWeb);                  // Envío de datos por JSON a la PagWeb
          PagWeb.println();                                   // Salto de línea para delimitar

          Serial.println("Fin de la prueba de Inv Polaridad");
          break;

        // Lectura Nominal de Corriente
        case 3:
          Serial.println("Lectura Nominal de Corriente");
          enviarJSON.clear();  // Limpia cualquier dato previo

          delay(20);
          corrienteSensor = medirCorriente();
          delay(20);

          Serial.print("Lectura Nominal: ");
          Serial.print(corrienteSensor, 3);
          Serial.println(" A");

          if (corrienteSensor > 2) {
            enviarJSON["status"] = "OK";  // Envio de corriente JSON para corto
          } else {
            enviarJSON["status"] = "Fail";
          }

          JSONCorriente = String(corrienteSensor, 1) + " A";  // Empaquetamiento
          enviarJSON["meas"] = JSONCorriente;                 // Envio de corriente JSON para corto
          serializeJson(enviarJSON, PagWeb);                  // Envío de datos por JSON a la PagWeb
          PagWeb.println();                                   // Salto de línea para delimitar
          break;

        // Lectura Nominal de Corriente en Break
        case 4:
          Serial.println("Lectura Nominal de Corriente en Break");
          enviarJSON.clear();  // Limpia cualquier dato previo

          delay(5);
          corrienteSensor = medirCorriente();
          delay(5);

          Serial.print("Lectura Break: ");
          Serial.print(corrienteSensor, 3);
          Serial.println(" A");

          if (fabs(corrienteSensor) < 0.2) {
            enviarJSON["status"] = "OK";
          } else {
            enviarJSON["status"] = "Fail";
          }

          JSONCorriente = String(corrienteSensor, 1) + " A";  // Empaquetamiento
          enviarJSON["meas"] = JSONCorriente;                 // Envio de corriente JSON para corto
          serializeJson(enviarJSON, PagWeb);                  // Envío de datos por JSON a la PagWeb
          PagWeb.println();                                   // Salto de línea para delimitar
          break;

        case 5:
          digitalWrite(RELAYA, LOW);  // Activo
          digitalWrite(RELAYB, LOW);  // Activo
          break;

        case 6:
          digitalWrite(RELAYA, HIGH);
          digitalWrite(RELAYB, HIGH);
          break;

        case 7:
          digitalWrite(RELAYCom, HIGH);
          break;

        case 8:
          digitalWrite(RELAYCom, LOW);
          break;

        case 9:
          digitalWrite(RELAYCom, HIGH);
          enviarJSON.clear();  // Limpia cualquier dato previo

          bool flagBat = true;

          for (int i = 0; i < 10; i++) {
            float corriente = medirCorriente();
            Serial.print("Medición ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(corriente);

            if (corriente < -0.1) {
              flagBat = false;
              enviarJSON["status"] = "OK";                  // Envio de corriente JSON
              JSONCorriente = String(corriente, 1) + " A";  // Empaquetamiento
              enviarJSON["meas"] = JSONCorriente;           // Envio de corriente JSON
              serializeJson(enviarJSON, PagWeb);            // Envío de datos por JSON a la PagWeb
              PagWeb.println();                             // Salto de línea para delimitar
              break;                                        // salir del for inmediatamente
            }

            delay(50);
          }

          if (flagBat) {
            float corriente = medirCorriente();
            enviarJSON["status"] = "Fail";                // Envio de corriente JSON
            JSONCorriente = String(corriente, 1) + " A";  // Empaquetamiento
            enviarJSON["meas"] = JSONCorriente;           // Envio de corriente JSON
            serializeJson(enviarJSON, PagWeb);            // Envío de datos por JSON a la PagWeb
            PagWeb.println();                             // Salto de línea para delimitar
          }

          break;
      }
    }

    else {
      Serial.print("Error de JSON: ");
      Serial.println(error.c_str());
    }
  }


  // Falso contacto
  else {
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, HIGH);
    digitalWrite(RELAY3, HIGH);
    digitalWrite(RELAY4, HIGH);
  }
}  // void loop()


// ## DECLARACIÓN DE FUNCIONES ##

// ==== Medición INA219 ====
float medirCorriente() {

  float shunt_mV = ina219.getShuntVoltage_mV();
  float bus_V = ina219.getBusVoltage_V();

  shunt_mV -= shuntOffset_mV;
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT;  // Corriente de interés
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;

  return current_A;
}
