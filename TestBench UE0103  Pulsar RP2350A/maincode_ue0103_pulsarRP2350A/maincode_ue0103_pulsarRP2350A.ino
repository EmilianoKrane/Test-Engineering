/*
Firmware Test blink pulsar rp2350a  controlado desde uart externo
*/


// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include "pico/unique_id.h"

#if defined(RP2350_PSRAM_CS)
#define PSRAM_TEST_SIZE 64                     // Solo 64 bytes - mínimo
uint8_t psramBuffer[1024] PSRAM;               // Solo 1KB en PSRAM (reducido de 256KB)
uint8_t psramTestData[PSRAM_TEST_SIZE] PSRAM;  // Test data también en PSRAM
#endif

// ==== DECLARACIÓN DE GPIOS ====
#define WS_PIN 1
#define D13_PIN 22  // >> LED BUILTIN
#define SDA_PIN 24  // >> GPIO06 Señal de datos en protocolo I2C
#define SCL_PIN 25  // >> GPIO07 Señal de reloj en protocolo I2C


// ==== DECLARACIÓN DE VARIABLES GLOBALES y MACROS ====
#define NUM_NEOP 3         // Cantidad de Neopixeles a controlar
#define OLED_RESET -1      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128   // OLED display width, in pixels
#define SCREEN_HEIGHT 64   // OLED display height, in pixels
bool status_OLED = false;  // Variable check de inicialización OLED por I2C
String JSON_entrada;       // Variable que recibe JSON del frontend
String JSON_salida;        // Variable que envía el JSON de datos

// Estado PSRAM
bool psramAvailable = false;
int psramSizeMB = 0;
int psramFreeKB = 0;
unsigned long lastPSRAMTest = 0;
unsigned long psramTestInterval = 60000;  // Test cada 60 segundos (muy reducido)
int psramTestsPassed = 0;
bool enablePSRAMTests = false;  // Deshabilitar tests por defecto para ahorrar RAMs

// ==== CREACIÓN DE OBJETOS ====
SerialPIO PagWeb(18, 19);                                                  // Sintaxis: SerialPIO nombre(TX_PIN, RX_PIN);
Adafruit_NeoPixel pixels(NUM_NEOP, WS_PIN, NEO_GRB + NEO_KHZ800);          // Objeto de Neopixel
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Objeto de la OLED
JsonDocument receiveJSON;
JsonDocument sendJSON;

// ==== FUNCIONES DE UTILIDAD ====
void pagwebDebug(String cmd) {
  JsonDocument doc;
  doc["debug"] = cmd;
  serializeJson(doc, PagWeb);
  PagWeb.println();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  PagWeb.begin(115200);

  delay(1000);
  mensajeBienvenida();
  pagwebDebug("¡Test PULSAR RP2350A Ready!");

  // ---- Asignación de entradas y salidas en GPIOs ----
  pinMode(D13_PIN, OUTPUT);

  pixels.begin();
  pixels.clear();
  pixels.show();
}

void loop() {

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);
    if (!error) {
      String Function = receiveJSON["Function"];
      int opc = 0;
      if (Function == "ping") opc = 1;          // {"Function":"ping"}
      else if (Function == "uid") opc = 2;      // {"Function":"uid"}
      else if (Function == "ram") opc = 3;      // {"Function":"ram"}
      else if (Function == "psram") opc = 4;    // {"Function":"psram"}
      else if (Function == "sd_test") opc = 5;  // {"Function":"sd_test"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println("");
            break;
          }

        case 2:
          {
            sendJSON.clear();
            String uid = "";
            pico_unique_board_id_t board_id;
            pico_get_unique_board_id(&board_id);
            for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
              if (board_id.id[i] < 0x10) {
                Serial.print("0");  // Agregar cero inicial manualmente
              }
              // Serial.print(board_id.id[i], HEX); B88CFFD62A6D293F
              uid += String(board_id.id[i], HEX);
            }
            // Serial.println("El uid es: " + uid);
            sendJSON["uid"] = uid;
            serializeJson(sendJSON, PagWeb);
            PagWeb.println("");
            break;
          }

        case 3:
          {
            sendJSON.clear();
            Serial.print("RAM libre inicial: ");
            Serial.print(rp2040.getFreeHeap() / 1024);
            Serial.println(" KB\n");

            String ram = String(rp2040.getFreeHeap() / 1024) + " KB";
            sendJSON["free_ram"] = ram;
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }

        case 4:
          {
            sendJSON.clear();

            Serial.print(">> Verificando PSRAM... ");
#if defined(RP2350_PSRAM_CS)
            psramSizeMB = rp2040.getPSRAMSize() / (1024 * 1024);
            Serial.println("Tamaño de la PSRAM " + String(psramSizeMB));
            if (psramSizeMB > 7) {
              psramFreeKB = rp2040.getFreePSRAMHeap() / 1024;
              psramAvailable = true;
              Serial.print("OK");
              Serial.print(psramSizeMB);
              Serial.print(" MB, ");
              Serial.print(psramFreeKB);
              Serial.println(" KB free");

              String psram = String(psramFreeKB) + " KB";
              sendJSON["Result"] = "OK";
              sendJSON["psram"] = psram;
              serializeJson(sendJSON, PagWeb);
              PagWeb.println();
              // Test inicial rápido (deshabilitado por defecto)
              // if (testPSRAM()) {
              //   psramTestsPassed++;
              //   Serial.println("   Test inicial: PASS");
              // }
            } else {
              Serial.println("NO DISPONIBLE");
              psramAvailable = false;
              sendJSON["Result"] = "FAIL";
              serializeJson(sendJSON, PagWeb);
              PagWeb.println();
            }
#else
            Serial.println("NO CONFIGURADO (define RP2350_PSRAM_CS)");
            psramAvailable = false;
#endif

            break;
          }

        default:
          {
            sendJSON.clear();
            sendJSON["status"] = "FAIL";
            sendJSON["error"] = "Invalid function requested";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println("");
            break;
          }
      }
    }
  } else {
    demo();
  }
}

void mensajeBienvenida() {
  delay(1000);  // Pequeña pausa para que el usuario alcance a leerlo
  Serial.println("========================================================");
  Serial.println("*                                                      *");
  Serial.println("*    🚀 ¡HOLA! BIENVENIDO A TU PULSAR RP2350A 🚀      *");
  Serial.println("*                                                      *");
  Serial.println("========================================================");
  Serial.println("*  Tu nueva placa de desarrollo esta lista para usar.  *");
  Serial.println("*  Esperamos que la disfrutes al máximo! :D            *");
  Serial.println("========================================================");
  delay(1000);  // Pequeña pausa para que el usuario alcance a leerlo
  Serial.print("\n>> Iniciando proceso de hackeo de computadora");
  for (int i = 0; i < 10; i++) {
    Serial.print(".");
    delay(100);
  }
  Serial.print("\n>> Computadora hackeada");
  for (int i = 0; i < 3; i++) {
    Serial.print(".");
    delay(300);
  }
  Serial.print("\n>> Borrando archivos de sistema");
  for (int i = 0; i < 10; i++) {
    Serial.print(".");
    delay(100);
  }
  delay(1000);
  Serial.println("\n Bromita! XDD");
  Serial.println("¡Disfruta de tu nueva PULSAR RP2350A de 🚀 UNIT Electronics 🚀 :D!");
}


void demo() {
  int delay_ms = 150;
  int i = 10;

  digitalWrite(D13_PIN, HIGH);
  pixels.setPixelColor(0, pixels.Color(i, 0, 0));
  pixels.setPixelColor(1, pixels.Color(0, i, 0));
  pixels.setPixelColor(2, pixels.Color(0, 0, i));
  pixels.show();
  delay(delay_ms);

  digitalWrite(D13_PIN, LOW);
  pixels.setPixelColor(0, pixels.Color(0, i, 0));
  pixels.setPixelColor(1, pixels.Color(0, 0, i));
  pixels.setPixelColor(2, pixels.Color(i, 0, 0));
  pixels.show();
  delay(delay_ms);

  digitalWrite(D13_PIN, HIGH);
  pixels.setPixelColor(0, pixels.Color(0, 0, i));
  pixels.setPixelColor(1, pixels.Color(i, 0, 0));
  pixels.setPixelColor(2, pixels.Color(0, i, 0));
  pixels.show();
  delay(delay_ms);

  digitalWrite(D13_PIN, LOW);
  pixels.setPixelColor(0, pixels.Color(0, i, 0));
  pixels.setPixelColor(1, pixels.Color(0, 0, i));
  pixels.setPixelColor(2, pixels.Color(i, 0, 0));
  pixels.show();
  delay(delay_ms);
}
