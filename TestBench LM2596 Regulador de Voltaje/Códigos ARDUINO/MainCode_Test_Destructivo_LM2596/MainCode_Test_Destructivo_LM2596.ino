/*



*/

// ==== BIBLIOTECAS ====
#include <Arduino.h>
#include <Adafruit_INA219.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <Wire.h>

// ==== DECLARACIÓN DE PINES ====
#define RUN_BUTTON 4  // >> GPIO04 Botonera de Arranque - Pin para botón de inicio físico
#define SDA_PIN 6     // >> GPIO06 SDA para I2C con el esclavo - Línea de datos I2C
#define SCL_PIN 7     // >> GPIO07 SCL para I2C con el esclavo - Línea de reloj I2C
#define RX2 15        // >> GPIO15 como RX de UART2 - Recepción de datos desde interfaz web
#define TX2 19        // >> GPIO19 como TX de UART2 - Transmisión de datos a interfaz web
#define RELAY1 14     // >> GPIO14 Accionamiento de Relé IN1 de Cortocircuito
#define RELAY2 0      // >> GPIO00 Accionamiento de Relé IN2 de Cortocircuito
#define RELAYA 8      // >> GPIO08 Accionamiento de Relé A Fuente de Alimentación [+]
#define RELAYB 9      // >> GPIO09 Accionamiento de Relé B Fuente de Alimentación [-]
// Nota: El GPIO08, por diseño, no cambia de estado, por lo que fisicamente en el
// TesBench se enceuntran puenteados RELAYA y RELAYB

// ==== CREACIÓN DE OBJETOS ====
HardwareSerial PagWeb(1);  // Crear objeto para UART2 en PULSAR como PagWeb
TwoWire I2CBus = TwoWire(0);
Adafruit_INA219 ina219_in(0x40);   // >> Sensor de Corriente a la entrada del TestBench 0x40
Adafruit_INA219 ina219_out(0x41);  // >> Sensor de Corriente a la salida del TestBench 0x41

String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_salida;                 ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas

// ==== DECLARACIÓN DE VARIABLES GLOBALES ====
const float shuntOffset_mV = 0.0;  // Offset en vacío para lectura inicial
const float R_SHUNT = 0.05;        // Resistencia Shunt = 50 mΩ
float corrienteSensor = 0;         // Variable de lectura de corriente con el sensor
float voltajeSensor = 0;           // Variable de lectura de voltaje con el sensor

void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas para JSON válido
  Serial.println("{\"debug\": \"" + str + "\"}");
}

void pagwebDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  PagWeb.println("{\"debug\": \"" + str + "\"}");
}

float current_in() {
  float shunt_mV = ina219_in.getShuntVoltage_mV();
  float bus_V = ina219_in.getBusVoltage_V();

  shunt_mV -= shuntOffset_mV;
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT;  // Corriente de interés
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;
  return current_A;
}

float current_out() {
  float shunt_mV = ina219_out.getShuntVoltage_mV();
  float bus_V = ina219_out.getBusVoltage_V();

  shunt_mV -= shuntOffset_mV;
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT;  // Corriente de interés
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;
  return current_A;
}

void setup() {

  // ==== Inicialización de Comunicación Serie ====
  Serial.begin(115200);
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  delay(100);
  serialDebug("Serial Initialized...");


  // ==== Inicialización de Bus I2C ====
  I2CBus.begin(SDA_PIN, SCL_PIN);
  if (!ina219_out.begin(&I2CBus)) {
    serialDebug("Current sensor INA219_out 0x41 no initilized...");
    pagwebDebug("Current sensor INA219_out 0x41 no initilized...");
    while (1) { delay(10); }
  }
  pagwebDebug("Test LM2596 StepUp Ready...");

  // ==== Configuración de GPIOS ====
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);
  pinMode(RUN_BUTTON, INPUT);

  digitalWrite(RELAY1, HIGH);  // >> Relevador de Cortocircuito OFF (Activo BAJAS)
  digitalWrite(RELAY2, HIGH);  // >> Relevador de Cortocircuito OFF (Activo BAJAS)
  digitalWrite(RELAYA, LOW);   // >> Relevador de Fuente ON (Activo BAJAS)
  digitalWrite(RELAYB, LOW);   // >> Relevador de Fuente ON (Activo BAJAS)
}



void loop() {

  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();
    delay(100);
    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Accionamiento por botonera");
      sendJSON["Run"] = "OK";
      serializeJson(sendJSON, PagWeb);
      PagWeb.println();
    }
  }

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');  // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {

      // ==== Claves de control JSON a recibir ===
      String Function = receiveJSON["Function"];
      int time = receiveJSON["time"];
      int iter = receiveJSON["iter"];
      int btw = receiveJSON["timebtw"];

      int opc = 0;
      if (Function == "shortCircuit") opc = 1;         // {"Function": "shortCircuit"}
      else if (Function == "nominalCurrent") opc = 2;  // {"Function": "nominalCurrent"}
      else if (Function == "loopShort") opc = 3;       // {"Function":"loopShort","time": 1000, "timebtw":1000, "iter": 20}

      switch (opc) {

        // Prueba de cortocircuito
        case 1:
          sendJSON.clear();  // Limpia cualquier dato previo
          serialDebug("Ejecución de prueba de Cortocircuito");

          // Accionamiento de relevadores
          digitalWrite(RELAY1, LOW);  // Activo
          digitalWrite(RELAY2, LOW);  // Activo
          delay(1000);

          corrienteSensor = current_out();
          delay(100);

          digitalWrite(RELAY1, HIGH);  // Apagado
          digitalWrite(RELAY2, HIGH);  // Apagado

          serialDebug("Corriente - " + String(corrienteSensor, 3));

          if (fabs(corrienteSensor) < 0.2) {
            sendJSON["Result"] = "OK";  // Clave JSON OK
          }

          JSON_salida = String(corrienteSensor, 3) + " A";  // Empaquetamiento
          sendJSON["LecturaCorto"] = JSON_salida;           // Envio de corriente JSON para corto
          serializeJson(sendJSON, PagWeb);                  // Envío de datos por JSON a la PagWeb
          PagWeb.println();                                 // Salto de línea para delimitar

          serialDebug("Fin de la prueba de corto");
          break;

        // Lectura nominal de corriente
        case 2:
          serialDebug("Ejecución de Lectura Nominal");
          sendJSON.clear();  // Limpia cualquier dato previo

          delay(100);
          corrienteSensor = current_out();

          Serial.print("Corriente Nominal: ");
          Serial.print(corrienteSensor, 3);
          Serial.println(" A");

          JSON_salida = String(corrienteSensor, 3) + " A";  // Empaquetamiento
          sendJSON["Lectura"] = JSON_salida;                // Envio de corriente JSON

          if (corrienteSensor > 2.5) {
            sendJSON["Result"] = "OK";  // Clave JSON OK
          }

          serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
          PagWeb.println();                 // Salto de línea para delimitar
          break;


        case 3:
          {
            sendJSON.clear();
            //int iter = iter;
            int delay_short = time;
            int delay_btw = btw;

            for (int i = 0; i < iter; i++) {
              // delay(delay_btw);
              digitalWrite(RELAY1, LOW);  // Activo
              digitalWrite(RELAY2, LOW);  // Activo
              delay(delay_short);

              corrienteSensor = current_out();
              delay(delay_btw);

              digitalWrite(RELAY1, HIGH);  // Apagado
              digitalWrite(RELAY2, HIGH);  // Apagado
              pagwebDebug("Corriente -> " + String(corrienteSensor) + " A | iter " + String(i));
              delay(delay_btw);
            }
            pagwebDebug("Fin del loop");
            break;
          }

          
      }
    }
  }
}
