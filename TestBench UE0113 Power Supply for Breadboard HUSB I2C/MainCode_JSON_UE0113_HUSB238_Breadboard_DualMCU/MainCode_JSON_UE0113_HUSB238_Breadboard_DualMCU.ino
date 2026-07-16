/**
 * ============================================================================
 * TestBench UE0084 - HUSB238 USB-C Power Delivery Module
 * ============================================================================
 * * Descripción:
 * Implementación en DualMCU RP2040
 * Código de integración para el testbench del módulo HUSB238.
 * Permite control remoto mediante JSON a través de PagWeb.
 * * Modos de operación:
 * - SWEEP:   Barrido automático de voltajes (5V, 9V, 12V, 15V, 20V) cada 2 segundos
 * - FIXED:   Establecimiento de voltaje fijo para mediciones precisas
 * - PING:    Verificación de conectividad
 * - INIT:    Inicialización del módulo HUSB238
 * - RELAY:   Control del relevador para regulador 3.3V
 * * Autor: Emiliano Molina
 * Fecha: 2026
 * ============================================================================
 */

#include <Wire.h>
#include "Adafruit_HUSB238.h"
#include <Arduino.h>
#include <ArduinoJson.h>

// ============================================================================
// DEFINICIONES DE PINES Y CONSTANTES (ADAPTADO A RP2040)
// ============================================================================
#define RX2 1         ///< GPIO1 - RX de la UART1 (o UART0 según mapeo)
#define TX2 0         ///< GPIO0 - TX de la UART1 (o UART0 según mapeo)
#define RUN_BUTTON 4  ///< GPIO4 - Botón de arranque del TestBench
#define I2C_SDA 12    ///< GPIO20 - PIN SDA para comunicación I2C con HUSB238
#define I2C_SCL 13    ///< GPIO21 - PIN SCL para comunicación I2C con HUSB238
#define RELAY 25      ///< GPIO17 - Control del relevador para regulador 3.3V

// ============================================================================
// DECLARACIÓN DE VARIABLES GLOBALES Y OBJETOS
// ============================================================================

// En el RP2040 usamos Serial1 (o Serial2) asignando los pines previamente.
#define PagWeb Serial1     ///< Interfaz UART para comunicación con PagWeb
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
  Serial.begin(115200);  ///< Serial USB nativo para debugging (115200 baud)

  // Configuración de UART por hardware en RP2040 antes del begin
  PagWeb.setRX(RX2);
  PagWeb.setTX(TX2);
  PagWeb.begin(115200);  ///< UART física para comunicación con PagWeb

  delay(500);
  Serial.println("{\"debug\": \"Serial Inicializado en RP2040\"}");

  // Inicialización del bus I2C adaptado a la API del RP2040
  Wire.setSDA(I2C_SDA);
  Wire.setSCL(I2C_SCL);
  Wire.begin();

  // Configuración de GPIOs para control de entrada/salida
  pinMode(RUN_BUTTON, INPUT_PULLDOWN);  ///< Entrada: Botón de arranque
  pinMode(RELAY, OUTPUT);               ///< Salida: Control de relevador
  digitalWrite(RELAY, LOW);             ///< Relevador inicialmente desactivado
}

// ============================================================================
// BUCLE PRINCIPAL (LOOP)
// ============================================================================

void loop() {

  // ========== DETECCIÓN DEL BOTÓN DE ARRANQUE ==========
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
  // Se lee desde la interfaz serial que interactúa con la PagWeb (PagWeb)
  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');  ///< Lee una línea JSON completa
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {

      String Function = receiveJSON["Function"];  ///< Obtiene el comando a ejecutar
      String Value = receiveJSON["Value"] | "";   ///< Obtiene parámetro adicional (si existe)

      int opc = 0;
      if (Function == "ping") opc = 1;            // {"Function":"ping"}
      else if (Function == "init_husb") opc = 2;  // {"Function":"init_husb"}
      else if (Function == "sweep") opc = 3;      // {"Function":"sweep"}
      else if (Function == "fixed") opc = 4;      // {"Function":"fixed", "Value":"5"}
      else if (Function == "restart") opc = 5;    // {"Function":"restart"}
      else if (Function == "relayOn") opc = 6;    // {"Function":"relayOn"}
      else if (Function == "relayOff") opc = 7;   // {"Function":"relayOff"}

      switch (opc) {
        // ----- CASE 1: PING -----
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

        // ----- CASE 2: INIT_HUSB -----
        case 2:
          {
            for (int i = 0; i < 3; i++) {
              sendJSON.clear();

              if (husb238.begin(HUSB238_I2CADDR_DEFAULT, &Wire)) {
                //Serial.println("HUSB238 Inicializado...");
                sendJSON["Result"] = "OK";
                sendJSON["debug"] = "HSUB238 Inicializado";
                state_husb = true;
                break;
              } else {
                //Serial.println("HUSB238 NO inicializado...");
                sendJSON["debug"] = "HSUB238 No Inicializado";
                sendJSON["Result"] = "FAIL";
                state_husb = false;
              }
              delay(100);
            }

            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // ----- CASE 3: SWEEP -----
        case 3:
          {
            sendJSON.clear();
            if (state_husb) {
              Serial.println("Voltage Sweep HSUB...");

              int voltajes[] = { 5, 9, 12, 15, 20 };
              int delay_ms = 4000;

              for (int i = 0; i < 5; i++) {
                String cmd = "PD:SET " + String(voltajes[i]);
                handleSCPI(cmd);
                Serial.println("Voltaje en: " + String(voltajes[i]) + " V");

                sendJSON["debug"] = "Voltaje " + String(voltajes[i]) + " V";
                serializeJson(sendJSON, PagWeb);
                PagWeb.println();
                sendJSON.clear();

                delay(delay_ms);
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

        // ----- CASE 4: FIXED -----
        case 4:
          {
            sendJSON.clear();
            if (Value != "") {
              if (state_husb) {
                Serial.println("Configurando voltaje fijo a: " + Value + " V");
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

        // ----- CASE 5: RESTART (ADAPTADO A RP2040) -----
        case 5:
          {
            sendJSON.clear();
            Serial.println("Reinicio de dispositivo");
            sendJSON["debug"] = "Reinicio de dispositivo...";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            serializeJson(sendJSON, Serial);
            Serial.println();
            delay(500);

            rp2040.restart();  ///< Comando nativo del core de RP2040 para reset por hardware
            break;
          }

        // ----- CASE 6: RELAY ON -----
        case 6:
          {
            digitalWrite(RELAY, HIGH);
            delay(100);
            break;
          }

        // ----- CASE 7: RELAY OFF -----
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

void handleSCPI(String c) {
  c.trim();
  c.toUpperCase();

  if (c == "*IDN?") {
    Serial.println("UNIT-DEVLAB,HUSB238,USBPD,1.0");
  } else if (c == "STAT?") {
    Serial.println(husb238.isAttached() ? "ATTACHED" : "UNATTACHED");
  } else if (c == "PD:LIST?") {
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
  } else if (c == "PD:GET?") {
    Serial.print("PD=");
    Serial.println(husb238.getPDSrcVoltage());
  } else if (c.startsWith("PD:SET")) {
    int v = c.substring(6).toInt();
    HUSB238_PDSelection sel;

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

    if (husb238.isVoltageDetected(sel)) {
      husb238.selectPD(sel);
      husb238.requestPD();
      Serial.print("OK:SET ");
      Serial.print(v);
      Serial.println("V");
    } else {
      Serial.println("ERR:UNAVAILABLE");
    }
  } else if (c == "PD:SWEEP") {
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
        delay(1500);
      }
    }
    Serial.println("SWEEP DONE");
  } else if (c == "CURR:GET?") {
    HUSB238_CurrentSetting curr = husb238.getPDSrcCurrent();
    Serial.print("CURR=");
    printCurrentValue(curr);
    Serial.println();
  } else if (c.startsWith("CURR:MAX?")) {
    int v = c.substring(9).toInt();
    HUSB238_PDSelection sel;

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
  } else {
    Serial.println("ERR:UNKNOWN_CMD");
  }
}

// ============================================================================
// FUNCIONES AUXILIARES - CONVERSIÓN DE VALORES
// ============================================================================

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