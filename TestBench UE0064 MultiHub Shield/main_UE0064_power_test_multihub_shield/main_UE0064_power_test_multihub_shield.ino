/* 
Firmware MultiHub Shield - Prueba de Potencia
*/

#include <Wire.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_INA219.h>

// ==== DECLARACIÓN DE GPIOS ====
#define RUN_BUTTON 4  // >> Botonera de Arranque - Pin para botón de inicio físico
#define SDA_PIN 6     // >> SDA para I2C con el esclavo - Línea de datos I2C
#define SCL_PIN 7     // >> SCL para I2C con el esclavo - Línea de reloj I2C
#define RX2 15        // >> GPIO15 como RX de UART2 - Recepción de datos desde interfaz web
#define TX2 19        // >> GPIO19 como TX de UART2 - Transmisión de datos a interfaz web
#define RELAY_PIN 17  // >> GPIO17 D1TX0 Accionamiento de Relevador
#define RELAY_VCC 16  // >> GPIO D0 VCC de Parte Lógica para Relevador
#define PWM_PIN 2     // >> GPIO02 Control de PWM

// ==== CREACIÓN DE OBJETOS ====
HardwareSerial PagWeb(1);          // Crear objeto para UART2 en PULSAR como PagWeb
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
// ==== VARIABLES PARA CONTROL DEL BOTÓN ====
bool ultimoEstadoBoton = LOW;           // Memoria del estado anterior del botón
unsigned long ultimoTiempoRebote = 0;   // Temporizador para el antirrebote
const unsigned long tiempoRebote = 50;  // 50ms suele ser el estándar ideal


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
 * @brief Envía el JSON armado al puerto especificado y lo limpia
 * @param destino Referencia al puerto de salida (Serial, PagWeb, etc.)
 */
void flushBuffer(Print& destino) {
  serializeJson(sendJSON, destino);
  destino.println();  // Agregamos el salto de línea para que el receptor sepa que terminó
  sendJSON.clear();   // Limpiamos el buffer para el siguiente uso
}

/**
 * @brief Función de depuración para interfaz web
 * @param str Mensaje a enviar por UART a la interfaz web en formato JSON
 */
void pagwebDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  PagWeb.println("{\"debug\": \"" + str + "\"}");
}

void setup() {
  // ==== Inicialización de Comunicación Serie ====
  Serial.begin(115200);                        // Comunicación serial para depuración
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);  // UART para interfaz web
  delay(100);
  serialDebug("Serial Initialized...");
  pagwebDebug("Test MultiHub Shield Initialized...");

  // ==== Inicialización de BUS I2C ====
  Wire.begin(SDA_PIN, SCL_PIN);  // Iniciar I2C como maestro

  // ==== Declaración de GPIOS ====
  pinMode(RUN_BUTTON, INPUT_PULLDOWN);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RELAY_VCC, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(RELAY_VCC, HIGH);
  digitalWrite(PWM_PIN, HIGH);
}

void loop() {

  // ==== 1. MANEJO DEL BOTÓN DE ARRANQUE ====
  bool lecturaBoton = digitalRead(RUN_BUTTON);

  if (lecturaBoton != ultimoEstadoBoton) {
    ultimoTiempoRebote = millis();
  }

  if ((millis() - ultimoTiempoRebote) > tiempoRebote) {
    static bool estadoValidado = LOW;  // Guarda el estado real validado

    if (lecturaBoton != estadoValidado) {
      estadoValidado = lecturaBoton;

      // DETECCIÓN DE FLANCO DE SUBIDA: ¡Solo ejecuta cuando el botón se PRESIONA!
      if (estadoValidado == HIGH) {
        serialDebug("Arranque por botonera");
        sendJSON.clear();
        sendJSON["Run"] = "OK";
        flushBuffer(PagWeb);  // Usamos nuestra nueva función mejorada
      }
    }
  }
  ultimoEstadoBoton = lecturaBoton;


  // ==== 2. MANEJO DE COMUNICACIÓN CON EL FRONTEND ====
  if (PagWeb.available()) {
    JSON_entrada = PagWeb.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // ==== Claves de JSON compatibles ====
    String Function = receiveJSON["Function"];
    String state_relay = receiveJSON["state_relay"] | "OFF";
    String pwm_mode = receiveJSON["pwm_mode"] | "null";

    int opc = 0;
    if (Function == "ping") opc = 1;           // {"Function": "ping"}
    else if (Function == "pwm") opc = 2;       // {"Function":"pwm", "pwm_mode":"sweep"}
    else if (Function == "sw_relay") opc = 3;  // {"Function":"sw_relay", "state_relay":"ON"}

    switch (opc) {
      case 1:
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          flushBuffer(PagWeb);
          break;
        }

      case 2:
        {
          sendJSON.clear();
          int STEP = 10;
          int delay_sweep = 200;
          

          // ---- Control de velocidad (FADE) ----
          for (int duty = 255; duty >= 0; duty -= STEP) {
            analogWrite(PWM_PIN, duty);
            pagwebDebug("sweep_down: " + String(duty));
            delay(delay_sweep);
          }
          if (pwm_mode == "sweep") {
            for (int duty = 0; duty <= 255; duty += STEP) {
              analogWrite(PWM_PIN, duty);
              pagwebDebug("sweep_up: " + String(duty));
              delay(delay_sweep);
            }

            // Confirmación para el frontend
            sendJSON["status"] = "success";
            sendJSON["pwm_mode"] = "sweep_completed";
          }

          // ---- Arranque y paro de motor (SWITCH) ----
          else if (pwm_mode == "switch") {
            analogWrite(PWM_PIN, 250);
            delay(1000);
            analogWrite(PWM_PIN, 120);
            delay(100);
            analogWrite(PWM_PIN, 0);
            delay(2000);
            analogWrite(PWM_PIN, 120);
            delay(100);
            analogWrite(PWM_PIN, 250);

            // Confirmación para el frontend
            sendJSON["status"] = "success";
            sendJSON["pwm_mode"] = "switch_completed";
          }

          // ---- Error de argumentos ----
          else {
            sendJSON["error"] = "Argumento pwm_mode invalido o faltante";
          }

          // Despachamos el JSON final
          flushBuffer(PagWeb);
          break;
        }

      case 3:
        {
          sendJSON.clear();
          if (state_relay == "ON") {
            digitalWrite(RELAY_PIN, LOW);
            sendJSON["relay_status"] = "ON";
          } else {
            digitalWrite(RELAY_PIN, HIGH);
            sendJSON["relay_status"] = "OFF";
          }
          flushBuffer(PagWeb);
          break;
        }

      default: break;
    }
  }
}
