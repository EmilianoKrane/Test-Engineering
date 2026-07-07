/* 
Firmware MultiHub Shield - Prueba de Potencia
*/

#include <Wire.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_INA219.h>
#include "Adafruit_HUSB238.h"

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
Adafruit_HUSB238 husb238;              ///< Objeto del módulo HUSB238 para control USB PD
HardwareSerial PagWeb(1);              // Crear objeto para UART2 en PULSAR como PagWeb
TwoWire I2CBus = TwoWire(0);           // Instancia TCP/I2C reservada para uso futuro
Adafruit_INA219 ina219_in(0x40);       // Sensor de corriente INA219 en entrada del testbench
Adafruit_INA219 ina219_out(0x41);      // Sensor de corriente INA219 en salida del testbench
String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos
String JSON_salida;                    ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;     ///< Documento JSON para armar respuestas

// ==== DECLARACIÓN DE VARIABLES GLOBALES ====
const float shuntOffset_mV = 0.0;  // Offset en vacío para lectura inicial
const float R_SHUNT = 0.05;        // Resistencia Shunt = 50 mΩ
float corrienteSensor = 0;         // Variable de lectura de corriente con el sensor
float voltajeSensor = 0;           // Variable de lectura de voltaje con el sensor
String cmd = "";                   ///< Buffer para comandos SCPI
bool state_husb = false;           ///< Bandera de estado: inicialización del módulo HUSB238
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
    String Value = receiveJSON["Value"] | "";  ///< Obtiene parámetro adicional (si existe)


    int opc = 0;
    if (Function == "ping") opc = 1;            // {"Function": "ping"}
    else if (Function == "pwm") opc = 2;        // {"Function":"pwm", "pwm_mode":"sweep"}
    else if (Function == "sw_relay") opc = 3;   // {"Function":"sw_relay", "state_relay":"ON"}
    else if (Function == "init_husb") opc = 4;  // {"Function": "init_husb"} 
    else if (Function == "fixed") opc = 5;      // {"Function": "fixed", "Value": "5"}

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

      // ----- CASE 4: INIT_HUSB - Inicialización del módulo HUSB238 -----
      case 4:
        {
          // Intenta inicializar 3 veces antes de fallar
          for (int i = 0; i < 3; i++) {
            sendJSON.clear();

            if (husb238.begin(HUSB238_I2CADDR_DEFAULT, &Wire)) {
              Serial.println("HUSB238 Inicializado...");
              sendJSON["Result"] = "OK";
              sendJSON["debug"] = "HSUB238 Inicializado";
              state_husb = true;  ///< Activa bandera de inicialización
              break;
            } else {
              Serial.println("HUSB238 NO inicializado...");
              sendJSON["debug"] = "HSUB238 No Inicializado";
              sendJSON["Result"] = "FAIL";
              state_husb = false;
            }

            delay(100);  ///< Espera entre intentos
          }

          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      // ----- CASE 5: FIXED - Establecer voltaje fijo -----
      case 5:
        {
          sendJSON.clear();
          if (Value != "") {
            // Valida que el módulo esté inicializado
            if (state_husb) {
              Serial.println("Configurando voltaje fijo a " + Value + " V");

              // Envía comando SCPI para establecer voltaje
              String cmd = "PD:SET " + Value;
              handleSCPI(cmd);

              sendJSON["debug"] = "Voltaje fijo seteado a " + Value + " V";
            } else {
              Serial.println("HUSB238 NO inicializado...");
              sendJSON["debug"] = "Error: HSUB no inicializado";
            }

          } else {
            Serial.println("Error: JSON no contiene el valor de voltaje");
            sendJSON["debug"] = "Error: Falta parametro Value";
          }

          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }




      default: break;
    }
  }
}




// ============================================================================
// FUNCIONES DE UTILIDAD - PROTOCOLO SCPI
// ============================================================================

/**
 * Procesa comandos SCPI (Standard Commands for Programmable Instruments)
 * para control del módulo HUSB238.
 * 
 * Comandos soportados:
 *   - *IDN?        : Identificación del instrumento
 *   - STAT?        : Estado de conexión del PD
 *   - PD:LIST?     : Lista voltajes disponibles
 *   - PD:GET?      : Voltaje actual seleccionado
 *   - PD:SET <V>   : Establece voltaje (5,9,12,15,18,20)
 *   - PD:SWEEP     : Barrido de todos los voltajes disponibles
 *   - CURR:GET?    : Corriente actual detectada
 *   - CURR:MAX? <V>: Corriente máxima para voltaje V
 * 
 * @param c Comando SCPI a procesar (cadena de texto)
 */
void handleSCPI(String c) {
  c.trim();         ///< Elimina espacios en blanco
  c.toUpperCase();  ///< Convierte a mayúsculas

  // ===== CONSULTAS DE IDENTIFICACIÓN E INFORMACIÓN =====
  if (c == "*IDN?") {
    // Retorna identificación del dispositivo
    Serial.println("UNIT-DEVLAB,HUSB238,USBPD,1.0");
  }

  // ===== CONSULTAS DE ESTADO =====
  else if (c == "STAT?") {
    // Verifica si hay un adaptador USB PD conectado
    Serial.println(husb238.isAttached() ? "ATTACHED" : "UNATTACHED");
  }

  // ===== CONSULTAS DEL MÓDULO DE POWER DELIVERY =====
  else if (c == "PD:LIST?") {
    // Lista todos los voltajes disponibles en el adaptador conectado
    HUSB238_PDSelection voltages[] = { PD_SRC_5V, PD_SRC_9V, PD_SRC_12V,
                                       PD_SRC_15V, PD_SRC_18V, PD_SRC_20V };
    int voltageValues[] = { 5, 9, 12, 15, 18, 20 };

    for (int i = 0; i < 6; i++) {
      if (husb238.isVoltageDetected(voltages[i])) {
        Serial.print(voltageValues[i]);
        Serial.print("V ");
      }
    }
    Serial.println();
  }

  else if (c == "PD:GET?") {
    // Retorna el voltaje actualmente seleccionado
    Serial.print("PD=");
    Serial.println(husb238.getPDSrcVoltage());
  }

  // ===== CONFIGURACIÓN DE POWER DELIVERY =====
  else if (c.startsWith("PD:SET")) {
    // Establece un voltaje específico
    // Formato: "PD:SET 5" establece 5V

    int v = c.substring(6).toInt();  ///< Extrae el valor del voltaje
    HUSB238_PDSelection sel;

    // Mapea el valor entero al tipo de enumeración correspondiente
    switch (v) {
      case 5: sel = PD_SRC_5V; break;
      case 9: sel = PD_SRC_9V; break;
      case 12: sel = PD_SRC_12V; break;
      case 15: sel = PD_SRC_15V; break;
      case 18: sel = PD_SRC_18V; break;
      case 20: sel = PD_SRC_20V; break;
      default:
        Serial.println("ERR:INVALID_VOLTAGE");
        return;
    }

    // Verifica que el voltaje esté disponible antes de seleccionarlo
    if (husb238.isVoltageDetected(sel)) {
      husb238.selectPD(sel);  ///< Selecciona el voltaje
      husb238.requestPD();    ///< Solicita el voltaje al adaptador
      Serial.print("OK:SET ");
      Serial.print(v);
      Serial.println("V");
    } else {
      Serial.println("ERR:UNAVAILABLE");
    }
  }

  // ===== BARRIDO DE VOLTAJES =====
  else if (c == "PD:SWEEP") {
    // Realiza un barrido secuencial de todos los voltajes detectados
    HUSB238_PDSelection levels[] = {
      PD_SRC_5V, PD_SRC_9V, PD_SRC_12V,
      PD_SRC_15V, PD_SRC_18V, PD_SRC_20V
    };

    for (int i = 0; i < 6; i++) {
      if (husb238.isVoltageDetected(levels[i])) {
        husb238.selectPD(levels[i]);
        husb238.requestPD();
        Serial.print("SWEEP ");
        Serial.print((i + 1) * 5);
        Serial.println("V");
        delay(1500);  ///< Espera 1.5 segundos entre cambios de voltaje
      }
    }
    Serial.println("SWEEP DONE");
  }

  // ===== CONSULTAS DE CORRIENTE =====
  else if (c == "CURR:GET?") {
    // Retorna la corriente máxima disponible en el voltaje actual
    HUSB238_CurrentSetting curr = husb238.getPDSrcCurrent();
    Serial.print("CURR=");
    printCurrentValue(curr);
    Serial.println();
  }

  else if (c.startsWith("CURR:MAX?")) {
    // Retorna la corriente máxima disponible para un voltaje específico
    // Formato: "CURR:MAX? 5" retorna corriente máxima a 5V

    int v = c.substring(9).toInt();  ///< Extrae el valor del voltaje
    HUSB238_PDSelection sel;

    // Mapea el valor entero al tipo de enumeración
    switch (v) {
      case 5: sel = PD_SRC_5V; break;
      case 9: sel = PD_SRC_9V; break;
      case 12: sel = PD_SRC_12V; break;
      case 15: sel = PD_SRC_15V; break;
      case 18: sel = PD_SRC_18V; break;
      case 20: sel = PD_SRC_20V; break;
      default:
        Serial.println("ERR:INVALID_VOLTAGE");
        return;
    }

    // Verifica disponibilidad y retorna corriente máxima
    if (husb238.isVoltageDetected(sel)) {
      HUSB238_CurrentSetting curr = husb238.currentDetected(sel);
      Serial.print("MAX_CURR@");
      Serial.print(v);
      Serial.print("V=");
      printCurrentValue(curr);
      Serial.println();
    } else {
      Serial.println("ERR:UNAVAILABLE");
    }
  }

  // ===== ERROR: COMANDO DESCONOCIDO =====
  else {
    Serial.println("ERR:UNKNOWN_CMD");
  }
}



// ============================================================================
// FUNCIONES AUXILIARES - CONVERSIÓN DE VALORES
// ============================================================================

/**
 * Convierte un valor de corriente HUSB238_CurrentSetting a formato legible
 * en amperios y lo imprime en el puerto serial.
 * 
 * @param curr Enumeración con el valor de corriente a convertir
 */
void printCurrentValue(HUSB238_CurrentSetting curr) {
  switch (curr) {
    case CURRENT_0_5_A: Serial.print("0.5A"); break;
    case CURRENT_0_7_A: Serial.print("0.7A"); break;
    case CURRENT_1_0_A: Serial.print("1.0A"); break;
    case CURRENT_1_25_A: Serial.print("1.25A"); break;
    case CURRENT_1_5_A: Serial.print("1.5A"); break;
    case CURRENT_1_75_A: Serial.print("1.75A"); break;
    case CURRENT_2_0_A: Serial.print("2.0A"); break;
    case CURRENT_2_25_A: Serial.print("2.25A"); break;
    case CURRENT_2_50_A: Serial.print("2.50A"); break;
    case CURRENT_2_75_A: Serial.print("2.75A"); break;
    case CURRENT_3_0_A: Serial.print("3.0A"); break;
    case CURRENT_3_25_A: Serial.print("3.25A"); break;
    case CURRENT_3_5_A: Serial.print("3.5A"); break;
    case CURRENT_4_0_A: Serial.print("4.0A"); break;
    case CURRENT_4_5_A: Serial.print("4.5A"); break;
    case CURRENT_5_0_A: Serial.print("5.0A"); break;
    default: Serial.print("UNKNOWN"); break;
  }
}

/**
 * Convierte un valor de corriente HUSB238_CurrentSetting a formato legible
 * en amperios con espacio y lo imprime en el puerto serial.
 * 
 * Nota: Esta función es similar a printCurrentValue pero añade espacio
 * después del valor. Actualmente no se utiliza.
 * 
 * @param srcCurrent Enumeración con el valor de corriente a convertir
 */
void printCurrentSetting(HUSB238_CurrentSetting srcCurrent) {
  switch (srcCurrent) {
    case CURRENT_0_5_A: Serial.print("0.5A "); break;
    case CURRENT_0_7_A: Serial.print("0.7A "); break;
    case CURRENT_1_0_A: Serial.print("1.0A "); break;
    case CURRENT_1_25_A: Serial.print("1.25A "); break;
    case CURRENT_1_5_A: Serial.print("1.5A "); break;
    case CURRENT_1_75_A: Serial.print("1.75A "); break;
    case CURRENT_2_0_A: Serial.print("2.0A "); break;
    case CURRENT_2_25_A: Serial.print("2.25A "); break;
    case CURRENT_2_50_A: Serial.print("2.50A "); break;
    case CURRENT_2_75_A: Serial.print("2.75A "); break;
    case CURRENT_3_0_A: Serial.print("3.0A "); break;
    case CURRENT_3_25_A: Serial.print("3.25A "); break;
    case CURRENT_3_5_A: Serial.print("3.5A "); break;
    case CURRENT_4_0_A: Serial.print("4.0A "); break;
    case CURRENT_4_5_A: Serial.print("4.5A "); break;
    case CURRENT_5_0_A: Serial.print("5.0A "); break;
    default: break;
  }
}