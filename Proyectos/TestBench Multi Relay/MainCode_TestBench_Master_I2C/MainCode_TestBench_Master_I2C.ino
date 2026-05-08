#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <HardwareSerial.h>

// ==== DECLARACIÓN DE PINES ====
#define RUN_BUTTON 4  // >> Botonera de Arranque
#define SDA_PIN 6     // >> SDA para I2C con el esclavo
#define SCL_PIN 7     // >> SCL para I2C con el esclavo
#define RX2 15        // >> GPIO15 como RX de UART2
#define TX2 19        // >> GPIO19 como TX de UART2

// --- Dirección I2C base del esclavo ---
const uint8_t SLAVE_ADDR = 0x40;

// ==== CREACIÓN DE OBJETOS ====
HardwareSerial PagWeb(1);

String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;                ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas


void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas para JSON válido
  Serial.println("{\"debug\": \"" + str + "\"}");
}

void pagwebDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  PagWeb.println("{\"debug\": \"" + str + "\"}");
}

// --- Función para enviar comando I2C al esclavo ---
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

void setup() {
  // ==== Inicialización de Comunicación Serie ===
  Serial.begin(115200);
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  delay(100);
  serialDebug("Serial Initialized...");
  pagwebDebug("Test Initialized...");

  // ==== Inicialización de BUS I2C ====
  Wire.begin(SDA_PIN, SCL_PIN);
  serialDebug("I2C Maestro inicializado en SDA: " + String(SDA_PIN) + " SCL: " + String(SCL_PIN));

  // ==== Declaración de GPIOS ====
  pinMode(RUN_BUTTON, INPUT);

  delay(500);
}

void loop() {

  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);
    sendJSON.clear();  // Limpia cualquier dato previo

    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Arranque por botonera");
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
      PagWeb.println();
    }
  }


  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // ==== Claves de JSON a recibir ====
    String Function = receiveJSON["Function"];
    int channel = receiveJSON["channel"] | 0;

    int opc = 0;
    if (Function == "ping") opc = 1;            // {"Function": "ping"}
    else if (Function == "scanAddr") opc = 2;   // {"Function": "scanAddr"}
    else if (Function == "channelON") opc = 3;  // {"Function": "channelON", "channel":1}
    else if (Function == "sweep") opc = 4;      // {"Function": "sweep"}
    else if (Function == "sleep") opc = 5;      // {"Function": "sleep"}

    switch (opc) {
      case 1:
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 2:
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

      case 3:
        {
          if (channel >= 1 && channel <= 16) {
            serialDebug("Test Channel " + String(channel) + " ON...");
            pagwebDebug("Test Channel " + String(channel) + " ON...");

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

      case 4:
        {
          pagwebDebug("Initiating test sweep...");
          sendCommandI2C(0xFF);
          delay(4000);  // Esperar a que termine el barrido
          sendCommandI2C(0xFE);
          break;
        }

      case 5:
        {
          pagwebDebug("Sleep mode...");
          sendCommandI2C(0xFE);
          delay(100);
          break;
        }

      default: break;
    }
  }
}
