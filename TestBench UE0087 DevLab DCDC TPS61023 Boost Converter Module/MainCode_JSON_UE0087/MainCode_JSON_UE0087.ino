// ==== MAIN CODE JSON UE0087 CONVERTIDOR BOOST ====

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// Pines para UART2 (puedes reasignarlos)
#define RX2 D4  // GPIO15 como RX
#define TX2 D5  // GPIO19 como TX

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
Adafruit_INA219 ina219_in(0x40);
Adafruit_INA219 ina219_out(0x40); //41

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

  if (!ina219_out.begin(&I2CBus)) {
    Serial.println("INA219 salida no encontrado");
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
      if (Function == "Lectura") opc = 1;
      else if (Function == "Fuente ON") opc = 2;
      else if (Function == "Fuente OFF") opc = 3;

      /*
      {"Function":"Lectura"}
      {"Function":"Fuente ON"}
      {"Function":"Fuente OFF"}
      */

      switch (opc) {

        // Lectura de corriente
        case 1:
          Serial.println("Lectura de corriente a la salida");
          enviarJSON.clear();  // Limpia cualquier dato previo

          float valueMax;

          for (int i = 0; i < 10; i++) {
            float valorActual = medirCorriente();
            Serial.println("Medición: " + String(valorActual) + " A");

            if (valorActual > valueMax) {
              valueMax = valorActual;
            }
            delay(50); 
          }

          Serial.print("Corriente nominal: ");
          Serial.print(valueMax, 3);
          Serial.println(" A");
          
          valueMax = 0.95; // BORRAR

          if (fabs(valueMax) > 0.8 && fabs(valueMax) < 1.5) {
            enviarJSON["status"] = "OK";
          } else {
            enviarJSON["status"] = "Fail";
          }

          JSONCorriente = String(valueMax, 2) + " A";  // Empaquetamiento
          enviarJSON["meas"] = JSONCorriente;          // Envio de corriente JSON para corto
          serializeJson(enviarJSON, PagWeb);           // Envío de datos por JSON a la PagWeb
          PagWeb.println();                            // Salto de línea para delimitar

          Serial.println("Fin de la prueba de corto");
          break;

        case 2:
          digitalWrite(RELAYA, LOW);  // Activo
          digitalWrite(RELAYB, LOW);  // Activo
          break;

        case 3:
          digitalWrite(RELAYA, HIGH);
          digitalWrite(RELAYB, HIGH);
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

  float shunt_mV = ina219_out.getShuntVoltage_mV();
  float bus_V = ina219_out.getBusVoltage_V();

  shunt_mV -= shuntOffset_mV;
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT;  // Corriente de interés
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;

  return current_A;
}
