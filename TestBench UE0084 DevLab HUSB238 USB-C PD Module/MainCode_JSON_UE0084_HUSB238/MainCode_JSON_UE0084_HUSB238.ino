/**
 * ============================================================================
 * TestBench UE0084 - HUSB238 USB-C Power Delivery Module
 * ============================================================================
 * 
 * Descripción:
 *   Código de integración para el testbench del módulo HUSB238.
 *   Permite control remoto mediante JSON a través de PagWeb.
 *   
 * Modos de operación:
 *   - SWEEP:   Barrido automático de voltajes (5V, 9V, 12V, 15V, 20V) cada 2 segundos
 *   - FIXED:   Establecimiento de voltaje fijo para mediciones precisas
 *   - PING:    Verificación de conectividad
 *   - INIT:    Inicialización del módulo HUSB238
 *   - RELAY:   Control del relevador para regulador 3.3V
 *   
 * Autor: Emiliano Molina
 * Fecha: 2026
 * ============================================================================
 */

#include <Wire.h>
#include "Adafruit_HUSB238.h"
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// ============================================================================
// DEFINICIONES DE PINES Y CONSTANTES
// ============================================================================

#define RX2 15        ///< GPIO15 - Recepción UART desde PagWeb
#define TX2 19        ///< GPIO19 - Transmisión UART hacia PagWeb
#define RUN_BUTTON 4  ///< GPIO4  - Botón de arranque del TestBench
#define I2C_SDA 6     ///< GPIO22 - PIN SDA para comunicación I2C con HUSB238
#define I2C_SCL 7     ///< GPIO23 - PIN SCL para comunicación I2C con HUSB238
#define RELAY 17      ///< GPIO17 - Control del relevador para regulador 3.3V

// ============================================================================
// DECLARACIÓN DE VARIABLES GLOBALES Y OBJETOS
// ============================================================================

HardwareSerial PagWeb(1);  ///< Interfaz UART para comunicación con PagWeb
Adafruit_HUSB238 husb238;  ///< Objeto del módulo HUSB238 para control USB PD

String JSON_entrada;                  ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<256> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;               ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<256> sendJSON;  ///< Documento JSON para armar respuestas

String cmd = "";          ///< Buffer para comandos SCPI
bool state_husb = false;  ///< Bandera de estado: inicialización del módulo HUSB238


// ============================================================================
// CONFIGURACIÓN INICIAL (SETUP)
// ============================================================================

void setup() {
  // Inicialización de puertos seriales
  Serial.begin(115200);                        ///< Serial para debugging (115200 baud)
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);  ///< UART para comunicación con PagWeb
  delay(500);
  Serial.println("{\"debug\": \"Serial Inicializado\"}");

  // Inicialización del bus I2C para interfaz con HUSB238
  Wire.begin(I2C_SDA, I2C_SCL);

  // Configuración de GPIOs para control de entrada/salida
  pinMode(RUN_BUTTON, INPUT);  ///< Entrada: Botón de arranque
  pinMode(RELAY, OUTPUT);      ///< Salida: Control de relevador
  digitalWrite(RELAY, LOW);    ///< Relevador inicialmente desactivado
}

// ============================================================================
// BUCLE PRINCIPAL (LOOP)
// ============================================================================

void loop() {

  // ========== DETECCIÓN DEL BOTÓN DE ARRANQUE ==========
  // Lee el estado del botón con debounce simple
  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();
    delay(100);
    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON["Run"] = "OK";           ///< Indica que se presionó el botón
      serializeJson(sendJSON, PagWeb);  ///< Envía confirmación a PagWeb
      PagWeb.println();
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
  }

  // ========== RECEPCIÓN Y PROCESAMIENTO DE COMANDOS JSON ==========
  // Verifica si hay datos disponibles en la UART de PagWeb
  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');  ///< Lee una línea JSON completa
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // Procesa el JSON solo si es válido
    if (!error) {

      String Function = receiveJSON["Function"];  ///< Obtiene el comando a ejecutar
      String Value = receiveJSON["Value"] | "";   ///< Obtiene parámetro adicional (si existe)

      // Mapea el comando JSON a un número de opción
      int opc = 0;
      if (Function == "ping") opc = 1;            ///< {"Function": "ping"} - Prueba de conectividad
      else if (Function == "init_husb") opc = 2;  ///< {"Function": "init_husb"} - Inicializa el módulo
      else if (Function == "sweep") opc = 3;      ///< {"Function": "sweep"} - Barrer voltajes: 5V->9V->12V->15V->20V
      else if (Function == "fixed") opc = 4;      ///< {"Function": "fixed", "Value": "5"} - Voltaje fijo (5,9,12,15,18,20)
      else if (Function == "restart") opc = 5;    ///< {"Function": "restart"} - Reinicia el dispositivo

      // ===== CONTROL DEL RELEVADOR PARA REGULADOR 3.3V =====
      else if (Function == "relayOn") opc = 6;   ///< {"Function": "relayOn"} - Activa el relevador
      else if (Function == "relayOff") opc = 7;  ///< {"Function": "relayOff"} - Desactiva el relevador

      switch (opc) {
        // ----- CASE 1: PING - Prueba de conectividad -----
        case 1:
          {
            sendJSON.clear();
            Serial.println("Comunicación PagWeb -> PULSAR OK");
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // ----- CASE 2: INIT_HUSB - Inicialización del módulo HUSB238 -----
        case 2:
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

        // ----- CASE 3: SWEEP - Barrido de voltajes -----
        case 3:
          {
            sendJSON.clear();
            if (state_husb) {
              Serial.println("Voltage Sweep HSUB...");

              // Array con voltajes a barrer (en orden ascendente)
              int voltajes[] = { 5, 9, 12, 15, 20 };
              int delay_ms = 4000;  ///< 2.2 segundos por cada voltaje

              // Realiza el barrido de voltajes
              for (int i = 0; i < 5; i++) {
                String cmd = "PD:SET " + String(voltajes[i]);
                handleSCPI(cmd);
                Serial.println("Voltaje en: " + String(voltajes[i]) + " V");

                // Notifica a PagWeb del voltaje actual
                sendJSON["debug"] = "Voltaje " + String(voltajes[i]) + " V";
                serializeJson(sendJSON, PagWeb);
                PagWeb.println();
                sendJSON.clear();

                delay(delay_ms);  ///< Espera 2 segundos antes del siguiente voltaje
              }

              Serial.println("Sweep finalizated...");
              sendJSON["debug"] = "Sweep finalizated";

            } else {
              Serial.println("HUSB238 NO inicializado...");
              sendJSON["debug"] = "HSUB no inicializado";
            }

            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // ----- CASE 4: FIXED - Establecer voltaje fijo -----
        case 4:
          {
            sendJSON.clear();
            if (Value != "") {
              // Valida que el módulo esté inicializado
              if (state_husb) {
                Serial.println("Configurando voltaje fijo a: " + Value + " V");

                // Envía comando SCPI para establecer voltaje
                String cmd = "PD:SET " + Value;
                handleSCPI(cmd);

                sendJSON["debug"] = "Voltaje fijo seteado a " + Value + " V";
              } else {
                Serial.println("HUSB238 NO inicializado...");
                sendJSON["debug"] = "Error: HSUB no inicializado";
              }

            } else {
              // Error: falta el parámetro Value
              Serial.println("Error: JSON no contiene el valor de voltaje");
              sendJSON["debug"] = "Error: Falta parametro Value";
            }

            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // ----- CASE 5: RESTART - Reinicia el dispositivo -----
        case 5:
          {
            ESP.restart();
            delay(1000);
            sendJSON.clear();
            Serial.println("Reinicio de dispositivo");
            sendJSON["debug"] = "Reinicio de dispositivo...";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // ----- CASE 6: RELAY ON - Activa el relevador -----
        case 6:
          {
            digitalWrite(RELAY, HIGH);
            delay(100);
            break;
          }

        // ----- CASE 7: RELAY OFF - Desactiva el relevador -----
        case 7:
          {
            digitalWrite(RELAY, LOW);
            delay(100);
            break;
          }

        default: break;
      }
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