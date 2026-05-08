/**
 * @file MainCode_TestBench_Master_I2C.ino
 * @brief Firmware del maestro I2C para TestBench Multi Relay
 *
 * Este código implementa el firmware para un dispositivo maestro (Pulsar C6) que controla
 * un multiplexador de relevadores a través de comunicación I2C con un esclavo ESP32C6.
 * El maestro recibe comandos desde una interfaz web vía UART y los traduce a comandos I2C.
 *
 * Funcionalidades principales:
 * - Control individual de relevadores (16 canales)
 * - Barrido automático de relevadores
 * - Modo de suspensión
 * - Escaneo de dispositivos I2C
 * - Comunicación JSON con interfaz web
 * - Control por botón físico
 *
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <HardwareSerial.h>

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
HardwareSerial PagWeb(1);  // UART para comunicación con interfaz web

String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;                ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas


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
 * @brief Envía un comando I2C al dispositivo esclavo
 * @param command Comando a enviar (byte)
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
 * @brief Función de configuración inicial del dispositivo
 * Inicializa comunicaciones seriales, I2C y configura pines GPIO
 */
void setup() {
  // ==== Inicialización de Comunicación Serie ====
  Serial.begin(115200);  // Comunicación serial para depuración
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);  // UART para interfaz web
  delay(100);
  serialDebug("Serial Initialized...");
  pagwebDebug("Test Initialized...");

  // ==== Inicialización de BUS I2C ====
  Wire.begin(SDA_PIN, SCL_PIN);  // Iniciar I2C como maestro
  serialDebug("I2C Maestro inicializado en SDA: " + String(SDA_PIN) + " SCL: " + String(SCL_PIN));

  // ==== Declaración de GPIOS ====
  pinMode(RUN_BUTTON, INPUT);  // Configurar botón como entrada

  delay(500);
}

/**
 * @brief Bucle principal del programa
 * Maneja la entrada del botón físico y procesa comandos JSON desde la interfaz web
 */
void loop() {

  // ==== Manejo del botón de arranque ====
  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);  // Debounce
    sendJSON.clear();  // Limpia cualquier dato previo

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
    if (Function == "ping") opc = 1;            // {"Function": "ping"}
    else if (Function == "scanAddr") opc = 2;   // {"Function": "scanAddr"}
    else if (Function == "channelON") opc = 3;  // {"Function": "channelON", "channel":1}
    else if (Function == "sweep") opc = 4;      // {"Function": "sweep"}
    else if (Function == "sleep") opc = 5;      // {"Function": "sleep"}

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
          delay(4000);  // Esperar a que termine el barrido
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

      default: break;
    }
  }
}
