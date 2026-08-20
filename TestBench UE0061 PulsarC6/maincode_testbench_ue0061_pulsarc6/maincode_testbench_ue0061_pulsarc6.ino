
/* 
Firmware testbench para pulsarc6
Est firmware funciona como puente entre la interfaz de pruebas y el target pulsar c6, enviando los comando JSON
que requiere el target para accionar cad uno de sus perifericos. 

-- Como conexiones, la PagWeb selecciona el COM de la Pulsar en el Test y el conector QWIIC del arnés
se conecta al bus de pines GPIO01 y GPIO02 UARTL
*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <Adafruit_INA219.h>

// ==== Declaración de pines
#define RUN_BUTTON 2  // Botón de Arranque
#define RX2 4         // GPIO como RXD
#define TX2 5         // GPIO como TXD
#define SDA_PIN 6     // >> SDA para I2C con el esclavo - Línea de datos I2C
#define SCL_PIN 7     // >> SCL para I2C con el esclavo - Línea de reloj I2C
#define RELAYA 8      // En corto con GPIO09
#define RELAYB 9      // Relevadores de Accionamiento de Fuente
#define RELAY_PIN 20  // >> GPIO de Accionamiento de Rele de Alimentación

#define RELAYA 8  // Relevadores de Accionamiento de Fuente
#define RELAYB 9

// ==== Inicialización de objetos
HardwareSerial UART(1);            // Objeto para UART2 en PULSAR como PagWeb
Adafruit_INA219 ina219_in(0x40);   // Sensor de corriente INA219 en entrada del testbench
Adafruit_INA219 ina219_out(0x41);  // Sensor de corriente INA219 en salida del testbench


// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

bool waitingResponse = false;
unsigned long sendTime = 0;
const unsigned long TIMEOUT = 3000;  // 3 segundos

const float shuntOffset_mV = 0.0;  // Offset en vacío para lectura inicial
const float R_SHUNT = 0.05;        // Resistencia Shunt = 50 mΩ
float corrienteSensor = 0;         // Variable de lectura de corriente con el sensor
float voltajeSensor = 0;           // Variable de lectura de voltaje con el sensor

String rxDIS = "";


// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  sendJSON.clear();
  StaticJsonDocument<255> doc;
  sendJSON["debug"] = str;
  serializeJson(sendJSON, Serial);
  Serial.println();
}

void pagwebDebug(String str) {
  sendJSON.clear();
  StaticJsonDocument<255> doc;
  sendJSON["debug"] = str;
  serializeJson(sendJSON, UART);
  UART.println();
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


void setup() {

  Serial.begin(115200);                      // Serial enlaza la PagWeb DIS 4800 || VSV 115200
  UART.begin(115200, SERIAL_8N1, RX2, TX2);  // Bus de comunicación con el CH552
  delay(200);
  serialDebug("Test Pulsar C6 Initialized...");

  Wire.begin(SDA_PIN, SCL_PIN);  // Iniciar I2C como maestro
  serialDebug("I2C inicializado en SDA: " + String(SDA_PIN) + " SCL: " + String(SCL_PIN));

  delay(200);
  if (!ina219_in.begin(&Wire)) {
    serialDebug("Current sensor INA219_out 0x40 no initilized...");
    while (1) {
      Serial.println(".");
      delay(10);
    }
  }
  serialDebug("Test Pulsar C6 Ready...");

  // ---- Definición de entradas y salidas ----
  pinMode(RUN_BUTTON, INPUT);
  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);
  digitalWrite(RELAY_PIN, HIGH);
}

void loop() {

  // ---- BOTÓN ----
  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);
    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON.clear();  // Limpia cualquier dato previo
      sendJSON["Run"] = "OK";
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
  }


  // ---- PagWeb → VSV ----
  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {
      String Function = receiveJSON["Function"];  // Function es la variable de interés del JSON
      int opc = 0;                                // Variable de switcheo

      if (Function == "ping") opc = 1;                // {"Function":"ping"}
      else if (Function == "mac") opc = 2;            // {"Function":"mac"}
      else if (Function == "gpios_test") opc = 3;     // {"Function":"gpios_test"}
      else if (Function == "relayON") opc = 4;        // {"Function":"relayON"}
      else if (Function == "relayOFF") opc = 5;       // {"Function":"relayOFF"}
      else if (Function == "currentSensor") opc = 6;  // {"Function":"currentSensor"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();  // Limpia cualquier dato previo
            sendJSON["Function"] = "ping";
            serializeJson(sendJSON, UART);  // Envío de datos por JSON a la PagWeb
            UART.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();
            sendJSON["Function"] = "mac";
            serializeJson(sendJSON, UART);  // Envío de datos por JSON a la PagWeb
            UART.println();
            break;
          }

        case 3:  // Testeo de Datos de GPIOs
          {
            sendJSON.clear();  // Limpia cualquier dato previo
            sendJSON["Function"] = "gpios_test";
            serializeJson(sendJSON, UART);  // Envío de datos por JSON a la PagWeb
            UART.println();
            break;
          }

        case 4:
          {
            digitalWrite(RELAY_PIN, HIGH);
            break;
          }

        case 5:
          {
            digitalWrite(RELAY_PIN, LOW);
            break;
          }

        case 6:
          {
            sendJSON.clear();
            float minCurrent = 0.150;
            float maxCurrent = 0.250;
            delay(50);
            float corrienteSensor = current_in();
            sendJSON["current"] = corrienteSensor;
            if (corrienteSensor > minCurrent && corrienteSensor < maxCurrent) sendJSON["Result"] = "OK";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }




        default:
          {
            sendJSON.clear();
            sendJSON["status"] = "FAIL";
            sendJSON["error"] = "Invalid function requested";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  }


  // ---- VSV → SERIAL (respuesta) ----
  if (UART.available()) {

    String rxLine = UART.readStringUntil('\n');
    StaticJsonDocument<200> filterJSON;
    DeserializationError error = deserializeJson(filterJSON, rxLine);

    if (!error) {

      serializeJson(filterJSON, Serial);
      Serial.println();
    }
    waitingResponse = false;
  }

  // ---- TIMEOUT ----
  if (waitingResponse && (millis() - sendTime >= TIMEOUT)) {
    waitingResponse = false;

    // Serial.println("{\"Result\":\"Fail\", \"uart\":\"Fail\", \"gpioIn\":\"Fail\", \"analog\":\"Fail\", \"sw\":\"Fail\", \"gpioOut\":\"Fail\"}");
  }
}