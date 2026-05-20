/* 
Est firmware funciona como puente e interprete por comunicación serial entre el testbench frontend y el menu de opciones
flasheado en la memoria del ATMega328 del proyecto DIS para validar gpios y diversas funcionalidades

-> La Pulsar funciona como un puente, reportando a la PagWeb|Frontend desde su serial nativo y enlanzando con el AtMega328
por medio del UART2 declarado en los GPIOS 4 y 5. Es importante que esta conexion de uart se haga al bus en 4 y 5
ya que, experimentalmente, se vio que el bus en D0 y D1 tiene interferencia en la comunicación al 
alimentar la tarjeta a 3.3V, a 5V funciona bien. 
Con esta observación, es importante recalcar que el tarjet se debe alimentar a 3.3V para que las salidas
del ATMega328 entreguen estados a este mismo voltaje y no afecten las entradas de la pulsar como test al 
sensar los gpios. 

*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de GPIOS ====
#define GPIO_CH1 2     // >> GPIO02 para lectura de estado digital en salida LED CH1 DIS PB1 -> D9
#define GPIO_LOD 3     // >> GPIO03 para lectura de estado digital en salida LED CARGA DIS PB2 -> D10
#define RX2 4          // >> GPIO D1 como RX del UART2 comunicado al ATMega328
#define TX2 5          // >> GPIO D0 como TX del UART2 comunicado al ATMega328
#define RUN_BUTTON 22  // >> Botonera de Arranque - Pin para botón de inicio físico

// ==== Inicialización de Objetos ====
HardwareSerial DIS(1);  // Bus de UART2 para comunicación con ATMega328

// ==== Estructura de JSON ====
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;
String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

// ==== Declaración de Variables Globales ====
bool waitingResponse = false;
unsigned long sendTime = 0;
const unsigned long TIMEOUT = 3000;  // 3 segundos
String rxDIS = "";

void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas para JSON válido
  Serial.println("{\"debug\": \"" + str + "\"}");
}

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
