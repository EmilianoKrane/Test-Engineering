/**
 * Firmware TestBench LM2596 Multi-Módulo
 *
 * Este firmware actúa como puente entre la interfaz de pruebas y el testbench,
 * permitiendo ejecutar pruebas sobre hasta 4 módulos reguladores step-up LM2596.
 *
 * Proporciona:
 *  - control por UART/JSON desde la interfaz web,
 *  - mediciones de corriente con sensores INA219,
 *  - control de relés para cortocircuito y alimentación,
 *  - pruebas individuales y barridos automáticos.
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <Adafruit_INA219.h>

// ==== DECLARACIÓN DE PINES ====
/**
 * Definición de pines GPIO utilizados en el hardware.
 * Estos pines están configurados para la comunicación I2C, UART y entrada de botón.
 */
#define RUN_BUTTON 4  // >> Botonera de Arranque - Pin para botón de inicio físico
#define SDA_PIN 6     // >> SDA para I2C con el esclavo - Línea de datos I2C
#define SCL_PIN 7     // >> SCL para I2C con el esclavo - Línea de reloj I2C
#define RX2 15        // >> GPIO15 como RX de UART2 - Recepción de datos desde interfaz web
#define TX2 19        // >> GPIO19 como TX de UART2 - Transmisión de datos a interfaz web
#define RELAY1 14     // >> GPIO14 Accionamiento de Relé IN1 de Cortocircuito
#define RELAY2 0      // >> GPIO00 Accionamiento de Relé IN2 de Cortocircuito
#define RELAYA 8      // >> GPIO08 Accionamiento de Relé A Fuente de Alimentación [+]
#define RELAYB 9      // >> GPIO09 Accionamiento de Relé B Fuente de Alimentación [-]
// Nota: El GPIO08, por diseño, no cambia de estado, por lo que fisicamente en el
// TesBench se encuentran puenteados RELAYA y RELAYB

// --- Dirección I2C base del esclavo ---
/**
 * Dirección I2C del dispositivo esclavo.
 * Esta dirección debe coincidir con la configurada en el firmware del esclavo.
 */
const uint8_t SLAVE_ADDR = 0x40;

// ==== CREACIÓN DE OBJETOS ====
/**
 * Objetos globales para manejo de comunicación serial y JSON.
 * PagWeb: Comunicación UART con la interfaz web
 * JSON buffers: Para parseo y creación de mensajes JSON
 */
HardwareSerial PagWeb(1);  // Crear objeto para UART2 en PULSAR como PagWeb
TwoWire I2CBus = TwoWire(0);    // Instancia TCP/I2C reservada para uso futuro
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
 * @brief Obtiene la corriente medida por el INA219 de entrada.
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
 * @brief Obtiene la corriente medida por el INA219 de salida.
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
  str.replace("\"", "\\\"");  // Escapa comillas para JSON válido
  Serial.println("{\"debug\": \"" + str + "\"}");
}

/**
 * @brief Función de depuración para interfaz web
 * @param str Mensaje a enviar por UART a la interfaz web en formato JSON
 */
void pagwebDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  PagWeb.println("{\"debug\": \"" + str + "\"}");
}

// --- Función para enviar comando I2C al esclavo ---
/**
 * @brief Envía un comando I2C al dispositivo esclavo.
 * @param command Comando a enviar (byte).
 */
void sendCommandI2C(uint8_t command) {
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(command);
  uint8_t error = Wire.endTransmission();

  if (error == 0) {
    serialDebug("Comando enviado OK: 0x" + String(command, HEX));
  } else {
    serialDebug("Error I2C " + String(error) + " enviando: 0x" + String(command, HEX));
  }
}

// --- Función para leer respuesta del esclavo ---
/**
 * @brief Lee una respuesta del dispositivo esclavo vía I2C
 * @return Respuesta del esclavo (byte) o 0xFF en caso de error
 */
uint8_t readResponseI2C() {
  Wire.requestFrom(SLAVE_ADDR, (uint8_t)1);  // Solicitar 1 byte

  if (Wire.available()) {
    uint8_t response = Wire.read();
    serialDebug("Respuesta recibida: 0x" + String(response, HEX));
    return response;
  } else {
    serialDebug("No hubo respuesta del esclavo");
    return 0xFF;  // Valor de error arbitrario
  }
}

/**
 * @brief Función de configuración inicial del dispositivo.
 * Inicializa comunicaciones seriales, I2C y configura pines GPIO.
 */
void setup() {
  // ==== Inicialización de Comunicación Serie ====
  Serial.begin(115200);                        // Comunicación serial para depuración
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);  // UART para interfaz web
  delay(100);
  serialDebug("Serial Initialized...");
  pagwebDebug("Test Multi LM2596 Initialized...");

  // ==== Inicialización de BUS I2C ====
  Wire.begin(SDA_PIN, SCL_PIN);  // Iniciar I2C como maestro
  serialDebug("I2C Maestro inicializado en SDA: " + String(SDA_PIN) + " SCL: " + String(SCL_PIN));

  //I2CBus.begin(SDA_PIN, SCL_PIN);
  if (!ina219_out.begin(&Wire)) {
    serialDebug("Current sensor INA219_out 0x41 no initilized...");
    pagwebDebug("Current sensor INA219_out 0x41 no initilized...");
    while (1) { delay(10); }
  }
  pagwebDebug("Test LM2596 StepUp Ready...");

  // ==== Declaración de GPIOS ====
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);
  pinMode(RUN_BUTTON, INPUT);

  digitalWrite(RELAY1, HIGH);  // >> Relevador de Cortocircuito OFF (Activo BAJAS)
  digitalWrite(RELAY2, HIGH);  // >> Relevador de Cortocircuito OFF (Activo BAJAS)
  digitalWrite(RELAYA, LOW);   // >> Relevador de Fuente ON (Activo BAJAS)
  digitalWrite(RELAYB, LOW);   // >> Relevador de Fuente ON (Activo BAJAS)
  delay(500);
}


/**
 * @brief Bucle principal del programa
 * Maneja la entrada del botón físico y procesa comandos JSON desde la interfaz web
 */
/**
void loop() {

  // ==== Manejo del botón de arranque ====
  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();  // Limpia cualquier dato previo
    delay(100);        // Debounce

    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Arranque por botonera");
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
      PagWeb.println();
    }
  }


  // ==== Procesamiento de comandos desde interfaz web ====
  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');  // Leer línea completa
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // ==== Claves de JSON a recibir ====
    /**
     * Procesamiento de comandos JSON recibidos:
     * - ping: Verificación de conectividad
     * - scanAddr: Escaneo de dispositivos I2C
     * - channelON: Activación de canal específico
     * - sweep: Barrido automático de relevadores
     * - sleep: Modo de suspensión
     */
    String Function = receiveJSON["Function"];
    int channel = receiveJSON["channel"] | 0;

    int opc = 0;
    if (Function == "ping") opc = 1;               // {"Function": "ping"}
    else if (Function == "scanAddr") opc = 2;      // {"Function": "scanAddr"}
    else if (Function == "channelON") opc = 3;     // {"Function": "channelON", "channel":1}
    else if (Function == "sweep") opc = 4;         // {"Function": "sweep"}
    else if (Function == "sleep") opc = 5;         // {"Function": "sleep"}
    else if (Function == "shortCircuit") opc = 6;  // {"Function": "shortCircuit"}

    switch (opc) {
      case 1:  // Ping
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
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
              pagwebDebug("I2C device found at 0x" + addrHex);
            }
          }
          break;
        }

      case 3:  // Activación de canal
        {
          if (channel >= 1 && channel <= 16) {
            serialDebug("Test Channel " + String(channel) + " ON...");
            pagwebDebug("Test Channel " + String(channel) + " ON...");

            // Mapear canal a comando I2C (0x00-0x0F para canales 1-16)
            switch (channel) {
              case 1: sendCommandI2C(0x00); break;
              case 2: sendCommandI2C(0x01); break;
              case 3: sendCommandI2C(0x02); break;
              case 4: sendCommandI2C(0x03); break;
              case 5: sendCommandI2C(0x04); break;
              case 6: sendCommandI2C(0x05); break;
              case 7: sendCommandI2C(0x06); break;
              case 8: sendCommandI2C(0x07); break;
              case 9: sendCommandI2C(0x08); break;
              case 10: sendCommandI2C(0x09); break;
              case 11: sendCommandI2C(0x0A); break;
              case 12: sendCommandI2C(0x0B); break;
              case 13: sendCommandI2C(0x0C); break;
              case 14: sendCommandI2C(0x0D); break;
              case 15: sendCommandI2C(0x0E); break;
              case 16: sendCommandI2C(0x0F); break;
              default: break;
            }
          } else {
            serialDebug("Invalid channel... Select [1-16]");
            pagwebDebug("Invalid channel... Select [1-16]");
          }
          delay(100);
          break;
        }

      case 4:  // Barrido automático
        {
          pagwebDebug("Initiating test sweep...");
          sendCommandI2C(0xFF);  // Comando de barrido
          delay(4000);           // Esperar a que termine el barrido
          sendCommandI2C(0xFE);  // Comando de suspensión
          break;
        }

      case 5:  // Modo suspensión
        {
          pagwebDebug("Sleep mode...");
          sendCommandI2C(0xFE);  // Comando de suspensión
          delay(100);
          break;
        }

      case 6:
        {
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
        }

      default: break;
    }
  }
}
