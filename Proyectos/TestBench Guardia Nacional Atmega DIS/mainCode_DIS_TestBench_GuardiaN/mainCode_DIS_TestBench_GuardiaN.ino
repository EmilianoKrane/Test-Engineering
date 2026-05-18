/* 
Est firmware funciona como puente e interprete por comunicación serial entre el testbench frontend y el menu de opciones
flasheado en la memoria del ATMega328 del proyecto DIS para validar gpios y diversas funcionalidades


-> La Pulsar funciona como un puente, reportando a la PagWeb|Frontend desde su serial nativo y enlanzando con el AtMega328
por medio del UART2 declarado en los GPIOS D0 y D1
*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de GPIOS ====
#define RX2 D1        // >> GPIO D1 como RX del UART2 comunicado al ATMega328
#define TX2 D0        // >> GPIO D0 como TX del UART2 comunicado al ATMega328
#define RUN_BUTTON 4  // >> Botonera de Arranque - Pin para botón de inicio físico

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

  // ==== COMUNICACIÓN UART PULSAR  -> DIS ====
  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (error) {
      serialDebug(String("Error JSON: ") + error.c_str());
      return;  // Aborta esta iteración del loop para no procesar basura
    } else {

      String Function = receiveJSON["Function"];
      int R = receiveJSON["R"] | 100;
      int G = receiveJSON["G"] | 0;
      int B = receiveJSON["B"] | 0;

      int opc = 0;
      if (Function == "ping") opc = 1;           // {"Function":"ping"}
      else if (Function == "help") opc = 2;      // {"Function":"help"}
      else if (Function == "neop_ON") opc = 3;   // {"Function":"neop_ON", "R": 100, "G": 100, "B": 100}
      else if (Function == "neop_OFF") opc = 4;  // {"Function":"neop_OFF"}


      switch (opc) {
        case 1:  // >> Validación de comunicación UART
          {
            sendJSON.clear();
            serialDebug("Initialized uart communication bus testing...");

            bool stateUART = false;
            bool foundcmd = false;
            int timeoutLimpieza = 100;
            while (DIS.available() && timeoutLimpieza > 0) {
              DIS.read();
              timeoutLimpieza--;
            }

            for (int i = 0; i < 10; i++) {
              while (DIS.available()) {
                String cmd = DIS.readStringUntil('\n');
                cmd.trim();
                if (cmd.indexOf("IN") != -1) foundcmd = true;
              }

              if (foundcmd) {
                sendJSON["Result"] = "OK";
                serializeJson(sendJSON, Serial);
                Serial.println();

                int timeoutLimpieza = 100;
                while (DIS.available() && timeoutLimpieza > 0) {
                  DIS.read();
                  timeoutLimpieza--;
                }

                stateUART = true;
                break;  // Rompe el ciclo for porque ya se encontraron las claves
              }

              delay(100);
            }

            if (!stateUART) {
              serialDebug("There is not communication via UART bus...");
            }
            break;
          }

        case 2:  // >> Despliegue de menú de ayuda
          {
            DIS.println("h");  // Solicitamos el menú

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
            serialDebug("Neopixel test initialized...");
            String cmd = "neon";
            DIS.println(cmd);
            delay(50);
            String color = "neo" + String(R) + "," + String(G) + "," + String(B);  // Armado de string para activar neopixel
            serialDebug(color);
            DIS.println(color);
            break;
          }

        case 4:
          {
            String cmd = "neooff";
            DIS.println(cmd);
            break;
          }


        default: break;
      }
    }


    /*
    char c = Serial.read();
    DIS.write(c);

    // Detectar envío del JSON específico
    rxDIS += c;
    if (rxDIS.endsWith("{\"Function\":\"testAll\"}")) {
      waitingResponse = true;
      sendTime = millis();
      rxDIS = "";  // limpiar buffer
    }

    // Evitar que crezca infinito
    if (rxDIS.length() > 64) rxDIS = "";
    */
  }
}
