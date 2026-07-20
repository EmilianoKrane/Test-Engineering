/**
 * 
 *
 * Este firmware actúa como 
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

// ==== DECLARACIÓN DE PINES ====
/**
 * Definición de pines GPIO utilizados en el hardware.
 * Estos pines están configurados para la comunicación I2C, UART y entrada de botón.
 */
#define RUN_BUTTON 4  // >> Botonera de Arranque - Pin para botón de inicio físico
#define SDA_PIN 6    // >> SDA para I2C con el esclavo - Línea de datos I2C
#define SCL_PIN 7    // >> SCL para I2C con el esclavo - Línea de reloj I2C


// ==== CREACIÓN DE OBJETOS ====
/**

 */

TwoWire I2CBus = TwoWire(0);       // Instancia TCP/I2C reservada para uso futuro
Adafruit_INA219 ina219_in(0x40);   // Sensor de corriente INA219 en entrada del testbench
Adafruit_INA219 ina219_out(0x41);  // Sensor de corriente INA219 en salida del testbench

String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_salida;                 ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas

// ==== DECLARACIÓN DE VARIABLES GLOBALES ====
const float shuntOffset_mV = 0.0;  // Offset en vacío para lectura inicial
const float R_SHUNT = 0.05;        // Resistencia Shunt = 50 mΩ
float corrienteSensor = 0;         // Variable de lectura de corriente con el sensor
float voltajeSensor = 0;           // Variable de lectura de voltaje con el sensor


/**
 * @brief Obtiene la corriente medida por el INA219 de entrada 0x40.
 * @return Corriente en amperios.
 */
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

/**
 * @brief Obtiene la corriente medida por el INA219 de salida 0x41.
 * @return Corriente en amperios.
 */
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


/**
 * @brief Función de depuración para Serial
 * @param str Mensaje a enviar por Serial en formato JSON
 */
void serialDebug(String str) {
  StaticJsonDocument<255> doc;
  doc["debug"] = str;
  serializeJson(doc, Serial);
  Serial.println("");
}


/**
 * @brief Función de configuración inicial del dispositivo.
 * Inicializa comunicaciones seriales, I2C y configura pines GPIO.
 */
void setup() {
  // ==== Inicialización de Comunicación Serie ====
  Serial.begin(115200);  // Comunicación serial para depuración
  delay(100);
  serialDebug("Serial Initialized...");

  // ==== Inicialización de BUS I2C ====
  Wire.begin(SDA_PIN, SCL_PIN);  // Iniciar I2C como maestro
  serialDebug("I2C inicializado en SDA: " + String(SDA_PIN) + " SCL: " + String(SCL_PIN));

  //I2CBus.begin(SDA_PIN, SCL_PIN);
  if (!ina219_in.begin(&Wire)) {
    serialDebug("Current sensor INA219_out 0x41 no initilized...");
  }

  // ==== Declaración de GPIOS ====
  pinMode(RUN_BUTTON, INPUT);
  delay(500);
}


/**
 * @brief Bucle principal del programa
 * Maneja la entrada del botón físico y procesa comandos JSON desde la interfaz web
 */

void loop() {

  // ==== Manejo del botón de arranque ====
  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();  // Limpia cualquier dato previo
    delay(100);        // Debounce

    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Arranque por botonera");
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, Serial);  // Envío de datos por JSON a la PagWeb
      Serial.println();
    }
  }


  // ==== Procesamiento de comandos desde interfaz web ====
  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');  // Leer línea completa
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // ==== Claves de JSON a recibir ====
    /**
     * Procesamiento de comandos JSON recibidos:
     * - ping: Verificación de conectividad
     * - scanAddr: Escaneo de dispositivos I2C
     */
    String Function = receiveJSON["Function"];
    int channel = receiveJSON["channel"] | 0;

    int opc = 0;
    if (Function == "ping") opc = 1;                // {"Function": "ping"}
    else if (Function == "scanAddr") opc = 2;       // {"Function": "scanAddr"}
    else if (Function == "currentSensor") opc = 3;  // {"Function": "currentSensor"}

    switch (opc) {
      case 1:  // Ping
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      case 2:  // Escaneo I2C
        {
          sendJSON.clear();
          for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
              String addrHex = "";
              if (addr < 16) addrHex = "0";
              addrHex = addrHex + String(addr, HEX);
              serialDebug("I2C device found at 0x" + addrHex);
            }
          }
          break;
        }

      case 3:
        {
          sendJSON.clear();
          float minCurrent = 0.4;
          float maxCurrent = 0.6;
          delay(50);
          corrienteSensor = current_in();
          serialDebug("Current " + String(corrienteSensor) + " A");
          bool state = corrienteSensor > minCurrent && corrienteSensor < maxCurrent;
          if (state) sendJSON["Result"] = "OK";
          else sendJSON["Result"] = "FAIL";
          sendJSON["current"] = String(corrienteSensor) + " A";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      default: break;
    }
  }
}