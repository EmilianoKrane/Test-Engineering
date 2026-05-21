/*
  Firmware puente Pulsar <=> Testbench DIS (Atmega328)

  Descripción general:
  --------------------
  Este firmware se carga en una placa Pulsar que actúa como puente entre el frontend
  (página web / PagWeb) y el firmware de test que corre en el target (Atmega328) del
  proyecto DIS de la Guardia Nacional. La Pulsar recibe comandos en JSON por su
  puerto serial nativo y traduce/encamina las solicitudes hacia el Atmega328 a través
  de un UART secundario (UART2). Además recoge respuestas y lecturas de pines para
  enviarlas de vuelta al frontend en formato JSON.

  Flujo y propósito:
  - La mini-PC del equipo i2d envía comandos y espera estados de gpio y otras pruebas.
  - La Pulsar reenvía comandos al Atmega328 (por UART2) y lee las respuestas.
  - Algunas operaciones exponen estados de pines digitales (GPIO_CH1, GPIO_LOD) y
    permiten activar/desactivar el neopixel o ejecutar pruebas automatizadas.

  Notas de hardware importantes:
  - El UART entre Pulsar y Atmega328 se configura en los GPIOS 4 (RX2) y 5 (TX2).
    Se observó interferencia al usar D0/D1 cuando el target está alimentado a 3.3V,
    por ello use RX/TX en 4/5 para evitar problemas.
  - El target debe alimentarse a 3.3V (multiprotocol alimenta el target a 3.3V).
  - GPIO 2 y 3 se usan para leer salidas digitales del target (LED CH1 y LED CARGA).
  - El arnés dispone de cables dupont y Qwiic para conexiones; el neopixel usa 3 cables:
    marrón = GND, rojo = VCC, naranja = DATA.

  Protocolos JSON soportados (ejemplos):
  - Ping:       {"Function":"ping"}
  - Manage:     {"Function":"manage","Action":"help"}
  - TestAll:    {"Function":"testAll"}
  - Neopixel ON:{"Function":"neop_ON","R":100,"G":100,"B":100}
  - Neopixel OFF:{"Function":"neop_OFF"}

  Comportamiento requerido: este archivo solo recibe comentarios explicativos.
  No se deben modificar secuencias de operación ni la lógica existente.
*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de GPIOS ====
// Pines usados por el arnés / testbench
#define GPIO_CH1 2     // Entrada: lectura digital salida LED CH1 del target (Atmega328)
#define GPIO_LOD 3     // Entrada: lectura digital salida LED CARGA del target
#define RX2 4          // RX del UART2 (Pulsar) conectado al TX del Atmega328
#define TX2 5          // TX del UART2 (Pulsar) conectado al RX del Atmega328
#define RUN_BUTTON 22  // Entrada: botón físico de arranque en el testbench

// ==== Inicialización de Objetos ====
// Instancia de HardwareSerial para UART2 (puerto usado para comunicarse con el Atmega328)
HardwareSerial DIS(1);

// ==== Estructura de JSON ====
// Buffers y documentos JSON para intercambio con el frontend
String JSON_entrada;                  // JSON crudo recibido desde el frontend (Serial nativo)
StaticJsonDocument<200> receiveJSON;  // Parse del JSON entrante
String JSON_salida;                   // JSON a enviar hacia el frontend
StaticJsonDocument<200> sendJSON;     // Document para serializar respuestas

// ==== Declaración de Variables Globales ====
// Variables de estado y temporización
bool waitingResponse = false;         // Flag genérico si se espera respuesta (no usado extensivamente)
unsigned long sendTime = 0;           // Marca temporal para timeouts
const unsigned long TIMEOUT = 3000;   // Timeout por defecto (ms)
String rxDIS = "";                  // Buffer temporal de recepción desde DIS

void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas para JSON válido
  Serial.println("{\"debug\": \"" + str + "\"}");
}

// serialDebug(): envía mensajes de depuración al frontend en formato JSON.
// Esto facilita que la página web reciba y muestre logs sin romper el protocolo JSON.

void setup() {

  Serial.begin(115200);                   // >> Serial nativo para comunicación con el Frontend
  DIS.begin(9600, SERIAL_8N1, RX2, TX2);  // >> Serial 2 para comunicación con el ATMega328
  delay(100);
  serialDebug("Test DIS Ready...");


  // ==== Declaración de Entradas/Salidas ====
  pinMode(RUN_BUTTON, INPUT);
  pinMode(GPIO_CH1, INPUT);
  pinMode(GPIO_LOD, INPUT);
}

// setup(): inicializa puertos seriales y configura pines como entradas.
// No se configuran salidas aquí porque la placa Pulsar actúa como lector/bridge.

void loop() {

  // ==== AARRANQUE POR BOTONERA ====
  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);
    sendJSON.clear();
    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON["Run"] = "OK";
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
  }

  // Nota: La lógica anterior detecta un pulso de inicio (botón) y notifica al frontend
  // enviando {"Run":"OK"}. El doble muestreo (HIGH -> delay -> LOW) sirve como anti-rebote simple.

  // ==== COMUNICACIÓN UART PULSAR <=> DIS ====
  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (error) {
      serialDebug(String("Error JSON: ") + error.c_str());
      return;  // Aborta esta iteración del loop para no procesar basura
    } else {

      String Function = receiveJSON["Function"];
      String Action = receiveJSON["Action"] | "help";
      int R = receiveJSON["R"] | 100;
      int G = receiveJSON["G"] | 0;
      int B = receiveJSON["B"] | 0;

      int opc = 0;
      if (Function == "ping") opc = 1;           // {"Function":"ping"}
      else if (Function == "manage") opc = 2;    // {"Function":"manage", "Action": "help"}
      else if (Function == "testAll") opc = 3;   // {"Function": "testAll"}
      else if (Function == "neop_ON") opc = 5;   // {"Function":"neop_ON", "R": 100, "G": 100, "B": 100}
      else if (Function == "neop_OFF") opc = 6;  // {"Function":"neop_OFF"}

      switch (opc) {
        case 1:  // >> Validación de comunicación UART
          {
            sendJSON.clear();
            serialDebug("Initialized uart communication bus testing...");

            bool stateUART = false;

            // 1. Limpieza inicial súper rápida
            while (DIS.available()) { DIS.read(); }

            String bufferRx = "";
            unsigned long startTime = millis();

            // 2. Abrimos una ventana de escucha activa de máximo 1.5 segundos
            while (millis() - startTime < 1500) {

              if (DIS.available()) {
                char c = DIS.read();
                bufferRx += c;  // Vamos armando el string byte por byte

                // 3. Evaluamos solo cuando detectamos que el mensaje terminó (salto de línea)
                if (c == '\n') {
                  bufferRx.trim();  // Limpiamos espacios o caracteres invisibles

                  // Usamos startsWith en lugar de indexOf para ser mucho más estrictos
                  if (bufferRx.startsWith("IN")) {
                    stateUART = true;
                    // --- NUEVA LÓGICA DE EXTRACCIÓN ---
                    // 1. Buscamos en qué posición de la cadena empieza el texto "PD2="
                    int indicePD2 = bufferRx.indexOf("PD2=");

                    // Si indexOf devuelve algo diferente a -1, significa que sí encontró el texto
                    if (indicePD2 != -1) {

                      // 2. "PD2=" tiene 4 caracteres. El valor (0 o 1) está 4 posiciones adelante del inicio
                      char valorPD2 = bufferRx.charAt(indicePD2 + 4);

                      // 3. Lo agregamos a tu JSON para que el frontend del testbench lo reciba
                      // Comparamos el caracter; si es '1', mandamos un entero 1, sino un 0.
                      sendJSON["PD2_status"] = (valorPD2 == '1') ? 1 : 0;
                    }
                    // ----------------------------------
                    break;  // ¡Encontramos un mensaje válido! Rompemos el while de inmediato
                  }

                  // Si llegó una línea completa pero no era la que buscábamos,
                  // limpiamos el buffer local y seguimos escuchando
                  bufferRx = "";
                }
              }
            }

            // 4. Reporte de resultados
            if (stateUART) {
              sendJSON["ping"] = "pong";
              serializeJson(sendJSON, Serial);
              Serial.println();
            } else {
              serialDebug("There is not communication via UART bus...");
            }

            // Limpieza final del hardware antes de salir
            while (DIS.available()) { DIS.read(); }
            break;
          }

        case 2:  // >> Despliegue de menú de ayuda
          {
            if (Action == "help") DIS.println("h");         // Solicitamos el menú
            else if (Action == "status") DIS.println("s");  // Solicitamos el estado de los GPIOS
            else return;

            unsigned long lastCharTime = millis();
            bool esperandoMenu = true;

            // 1. Esperamos a que el micro empiece a responder (Timeout de inicio: 1 segundo máximo)
            while (!DIS.available() && (millis() - lastCharTime < 1000)) {
              // Espera inactiva hasta que llegue el primer byte
            }

            // Reiniciamos el cronómetro justo cuando empieza a llegar la información
            lastCharTime = millis();

            // 2. Leemos todo el menú hasta que haya un silencio
            while (esperandoMenu) {
              if (DIS.available()) {
                Serial.write(DIS.read());  // Imprimimos el caracter en el monitor
                lastCharTime = millis();   // Reseteamos el cronómetro cada que llega un byte nuevo
              }

              // Si pasan 50 milisegundos sin recibir un solo caracter nuevo,
              // asumimos que el menú terminó de transmitirse por completo.
              if (millis() - lastCharTime > 50) {
                esperandoMenu = false;
              }
            }
            break;
          }

        case 3:
          {
            sendJSON.clear();
            bool stateLOD = false, stateCH1 = false;
            int delay_ms = 500;

            DIS.write("0");  // Comando para establecer todas las salidas en LOW
            delay(delay_ms);
            bool initLOD = digitalRead(GPIO_LOD);
            bool initCH1 = digitalRead(GPIO_CH1);
            delay(50);
            DIS.write("1");  // Comando para establecer todas las salidas en HIGH
            delay(delay_ms);
            bool finalLOD = digitalRead(GPIO_LOD);
            bool finalCH1 = digitalRead(GPIO_CH1);
            delay(50);
            DIS.write("0");  // Comando para establecer todas las salidas en LOW

            if (!initLOD && finalLOD && !initCH1 && finalCH1) sendJSON["Result"] = "OK";
            else sendJSON["gpios"] = "ERROR";
            serialDebug("LOD -> " + String(initLOD) + "|" + String(finalLOD));
            serialDebug("CH1 -> " + String(initCH1) + "|" + String(finalCH1));

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 5:
          {
            serialDebug("Neopixel test initialized...");
            String cmd = "neon";
            DIS.println(cmd);
            delay(50);
            String color = "neo" + String(R) + "," + String(G) + "," + String(B);  // Armado de string para activar neopixel
            serialDebug(color);
            DIS.println(color);
            break;
          }

        case 6:
          {
            String cmd = "neooff";
            DIS.println(cmd);
            break;
          }


        default: break;
      }
    }
  }
}

// Fin del archivo: la lógica del loop procesa comandos JSON entrantes desde el frontend,
// traduce acciones a comandos serie hacia el Atmega328 y reenvía lecturas/estados.
