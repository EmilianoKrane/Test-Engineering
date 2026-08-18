/*
ue0061 firmware test main pulsar c6
*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
// #include "DevLab_Test_PulsarC6.h"

// ==== DECLARACIÓN DE GPIOS ====
#define SDA_PIN 6   // >> GPIO06 Señal de datos en protocolo I2C
#define SCL_PIN 7   // >> GPIO07 Señal de reloj en protocolo I2C
#define NEOP_PIN 8  // >> GPIO08 Activación de Neopixel

#define D0_PIN D0
#define D1_PIN D1
#define D4_PIN 15
#define D5_PIN 19
#define D6_PIN 20
#define D7_PIN 21
#define D10_PIN 18
#define D12_PIN 2
#define D20_PIN 14
#define D14_PIN 0
#define D15_PIN 1
#define D16_PIN 3
#define D17_PIN 4
#define D18_PIN 22
#define D19_PIN 23
#define D21_PIN 5


// ==== DECLARACIÓN DE VARIABLES GLOBALES y MACROS ====
#define NUMPIXELS 26  // Número de Neopixeles en matrix
int intensity = 50;   // Intensidad de brillo en Neopixel

#define OLED_RESET -1      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128   // OLED display width, in pixels
#define SCREEN_HEIGHT 64   // OLED display height, in pixels
bool status_OLED = false;  // Variable check de inicialización OLED por I2Cs

String JSON_entrada;  // Variable que recibe JSON del frontend
String JSON_salida;   // Variable que envía el JSON de datos

const char *ssid = "IngPruebas-Master";
const char *password = "cachirula";
const char *serverUrl = "http://192.168.4.1/ping";

// ==== CREACIÓN DE OBJETOS ====
StaticJsonDocument<200> receiveJSON;
StaticJsonDocument<200> sendJSON;
Adafruit_NeoPixel pixels(NUMPIXELS, NEOP_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Objeto de la OLED

void setup() {
  // ---- Inicializaciones ----
  Serial.begin(115200);
  delay(100);

  // ---- rutina de chequeo de bloques de comunicación ----
  checkPulsar();  // Revisión de bloque i2c y spi



  // ---- Configuración de GPIOS Entrada/Salida ----
  pinMode(SDA_PIN, OUTPUT);

  // ---- Configuración del Neopixel ----
  pixels.clear();
  pixels.setBrightness(150);
}

void loop() {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "ping") opc = 1;             // {"Function":"ping"}
      else if (Function == "mac") opc = 2;         // {"Function":"mac"}
      else if (Function == "gpios_test") opc = 3;  // {"Function":"gpios_test"}
      else if (Function == "sd_test") opc = 4;     // {"Function":"sd_test"}
      else if (Function == "neop_test") opc = 5;   // {"Function":"neop_test"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();
            String mac = WiFi.macAddress();
            sendJSON["mac"] = mac;
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:
          {
            sendJSON.clear();

            if (status_OLED) {
              display.clearDisplay();
              display.setCursor(0, 0);
              display.setTextSize(1);
              display.print(F("Rutina: "));
              display.println("Check GPIOs");
              display.display();
            }

            bool stateA = testGpios(D0_PIN, D1_PIN);
            bool stateB = testGpios(D4_PIN, D5_PIN);
            bool stateC = testGpios(D6_PIN, D7_PIN);
            bool stateD = testGpios(D10_PIN, D12_PIN);
            bool stateE = testGpios(D20_PIN, D14_PIN);
            bool stateF = testGpios(D15_PIN, D16_PIN);
            bool stateG = testGpios(D17_PIN, D18_PIN);
            bool stateH = testGpios(D19_PIN, D21_PIN);

            if (stateA && stateB && stateC && stateD && stateE && stateF && stateG && stateH) {
              sendJSON["gpios"] = true;
              sendJSON["Result"] = "OK";
            } else {
              sendJSON["gpios"] = false;
              if (!stateA) sendJSON["D0,D1"] = "fail";
              if (!stateB) sendJSON["D4,D5"] = "fail";
              if (!stateC) sendJSON["D6,D7"] = "fail";
              if (!stateD) sendJSON["D10,D12"] = "fail";
              if (!stateE) sendJSON["D14,D20"] = "fail";
              if (!stateF) sendJSON["D15,D16"] = "fail";
              if (!stateG) sendJSON["D17,D18"] = "fail";
              if (!stateH) sendJSON["D19,D21"] = "fail";
            }

            if (stateA) sendJSON["D0,D1"] = "ok";
            if (stateB) sendJSON["D4,D5"] = "ok";
            if (stateC) sendJSON["D6,D7"] = "ok";
            if (stateD) sendJSON["D10,D12"] = "ok";
            if (stateE) sendJSON["D14,D20"] = "ok";
            if (stateF) sendJSON["D15,D16"] = "ok";
            if (stateG) sendJSON["D17,D18"] = "ok";
            if (stateH) sendJSON["D19,D21"] = "ok";

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 4:
          {
            sendJSON.clear();
            SPI.begin(SDA_PIN, D12_PIN, SCL_PIN, D5_PIN);

            if (!SD.begin(D5_PIN)) {
              Serial.println("Card Mount Failed");
              return;
            }

            uint8_t cardType = SD.cardType();

            if (cardType == CARD_NONE) {
              Serial.println("No SD card attached");
              return;
            }

            Serial.print("SD Card Type: ");
            if (cardType == CARD_MMC) {
              Serial.println("MMC");
            } else if (cardType == CARD_SD) {
              Serial.println("SDSC");
            } else if (cardType == CARD_SDHC) {
              Serial.println("SDHC");
            } else {
              Serial.println("UNKNOWN");
            }

            uint64_t cardSize = SD.cardSize() / (1024 * 1024);
            Serial.printf("SD Card Size: %lluMB\n", cardSize);

            listDir(SD, "/", 0);
            createDir(SD, "/mydir");
            listDir(SD, "/", 0);
            removeDir(SD, "/mydir");
            listDir(SD, "/", 2);
            writeFile(SD, "/hello.txt", "Hello ");
            appendFile(SD, "/hello.txt", "World!\n");
            readFile(SD, "/hello.txt");
            deleteFile(SD, "/foo.txt");
            renameFile(SD, "/hello.txt", "/foo.txt");
            readFile(SD, "/foo.txt");
            testFileIO(SD, "/test.txt");
            Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
            Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));

            break;
          }


        case 5:
          {
            sendJSON.clear();

            // ---- Neopixel en Rojo ----
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, pixels.Color(intensity, 0, 0));
              pixels.show();
              delay(100);
            }
            // ---- Neopixel en Verde ----
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, pixels.Color(0, intensity, 0));
              pixels.show();
              delay(100);
            }
            // ---- Neopixel en Azul----
            for (int i = 0; i < NUMPIXELS; i++) {
              pixels.setPixelColor(i, pixels.Color(0, 0, intensity));
              pixels.show();
              delay(100);
            }
            pixels.clear();
            break;
          }

        default:
          {
            sendJSON.clear();
            sendJSON["status"] = "FAIL";
            sendJSON["error"] = "Invalid function requested";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  } else {
    demo();
  }
}
void checkPulsar() {
  sendJSON.clear();
  sendJSON["System"] = "Ready";
  sendJSON["Module"] = "PulsarC6";

  bool statusI2C = false;
  bool statusSPI = false;
  String stateSPI = "FAIL";
  bool statusWiFi = false;

  // =========================================================================
  // 1. INICIALIZACIÓN I2C Y PANTALLA OLED
  // =========================================================================
  // Asegurar pines limpios antes de arrancar I2C
  gpio_reset_pin((gpio_num_t)SDA_PIN);
  gpio_reset_pin((gpio_num_t)SCL_PIN);
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(50);

  if (i2cCheckDevice(0x3C)) {
    statusI2C = true;
    sendJSON["i2c_bus"] = true;

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Encabezado
    display.setCursor(0, 0);
    display.println("Estado General:");

    // Estado Bus I2C
    display.setCursor(0, 16);
    display.print("Bus I2C: ");
    display.setCursor(65, 16);
    display.println("OK");

    // Placeholder Bus SPI
    display.setCursor(0, 26);
    display.print("Bus SPI: ");
    display.setCursor(65, 26);
    display.println("Test...");

    // Placeholder WiFi
    display.setCursor(0, 36);
    display.print("WiFi:    ");
    display.setCursor(65, 36);
    display.println("Wait...");

    display.display();
  } else {
    sendJSON["i2c_bus"] = false;
  }

  // =========================================================================
  // 2. LIBERAR I2C Y CAMBIAR A SPI
  // =========================================================================
  Wire.end();
  gpio_reset_pin((gpio_num_t)SDA_PIN);
  gpio_reset_pin((gpio_num_t)SCL_PIN);
  delay(50);  // Tiempo para que las líneas se estabilicen

  SPI.begin(SDA_PIN, D12_PIN, SCL_PIN, D5_PIN);

  if (SD.begin(D5_PIN)) {
    statusSPI = true;
    stateSPI = "OK";
    sendJSON["init_sd"] = true;
    sendJSON["spi_bus"] = true;

    uint8_t cardType = SD.cardType();
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);

    if (cardType == CARD_NONE) {
      sendJSON["sd_att"] = "No SD card attached";
    } else {
      if (cardType == CARD_MMC) sendJSON["sd_type"] = "MMC";
      if (cardType == CARD_SD) sendJSON["sd_type"] = "SDSC";
      if (cardType == CARD_SDHC) sendJSON["sd_type"] = "SDHC";
      sendJSON["sd_size"] = cardSize;
    }
  } else {
    sendJSON["init_sd"] = false;
    sendJSON["spi_bus"] = false;
  }

  // Liberar el hardware SPI
  SD.end();
  SPI.end();
  pinMode(D5_PIN, OUTPUT);
  digitalWrite(D5_PIN, HIGH);  // Forzar CS alto (deseleccionar módulo)

  // =========================================================================
  // 3. RECUPERAR I2C Y ACTUALIZAR OLED
  // =========================================================================
  gpio_reset_pin((gpio_num_t)SDA_PIN);
  gpio_reset_pin((gpio_num_t)SCL_PIN);
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);
  delay(50);

  if (statusI2C) {
    Wire.begin(SDA_PIN, SCL_PIN);

    // Sobrescribir "Test..." con el estado del SPI
    display.fillRect(65, 26, 60, 10, SSD1306_BLACK);
    display.setCursor(65, 26);
    display.println(stateSPI);

    // Cambiar estado WiFi de "Wait..." a "Check..."
    display.fillRect(65, 36, 60, 10, SSD1306_BLACK);
    display.setCursor(65, 36);
    display.println("Check...");
    display.display();
  }

  // =========================================================================
  // 4. CHEQUEO DE RED Y COMUNICACIÓN WIFI
  // =========================================================================
  WiFi.mode(WIFI_MODE_STA);
  String mac = WiFi.macAddress();
  sendJSON["mac"] = mac;

  int steps = 0;
  int attempts = 20;
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED && steps < attempts) {
    Serial.print(".");
    steps++;
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to test network!");

    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    JsonDocument docReq;
    docReq["device"] = "PulsarC6";
    docReq["message"] = "ping to validate connection";

    String jsonRequest;
    serializeJson(docReq, jsonRequest);

    int httpResponseCode = http.POST(jsonRequest);

    if (httpResponseCode > 0) {
      String payload = http.getString();
      JsonDocument docRes;
      DeserializationError error = deserializeJson(docRes, payload);

      if (!error && docRes["key"] == "pong") {
        statusWiFi = true;
        sendJSON["WiFi_status"] = true;
        sendJSON["WiFi_message"] = "pong";
      } else {
        sendJSON["WiFi_status"] = false;
        sendJSON["WiFi_error"] = "bad response/json";
      }
    } else {
      sendJSON["WiFi_status"] = false;
      sendJSON["WiFi_error"] = "HTTP failed: " + String(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("\nWiFi connection timed out");
    sendJSON["WiFi_status"] = false;
    sendJSON["WiFi_error"] = "no connection";
  }

  // =========================================================================
  // 5. ACTUALIZACIÓN FINAL DE WIFI EN OLED
  // =========================================================================
  if (statusI2C) {
    display.fillRect(65, 36, 60, 10, SSD1306_BLACK);
    display.setCursor(65, 36);
    display.println(statusWiFi ? "OK" : "FAIL");
    display.display();
  }

  // =========================================================================
  // 6. SALIDA JSON FINAL (Puerto Serial)
  // =========================================================================
  serializeJson(sendJSON, Serial);
  Serial.println();
}


void serialDebug(String str) {
  sendJSON.clear();
  sendJSON["debug"] = str;
  serializeJson(sendJSON, Serial);
  Serial.println();
}

bool testGpios(uint8_t gpioA, uint8_t gpioB) {
  bool resultAB = testSequence(gpioA, gpioB);
  if (!resultAB)
    return false;
  delay(10);  // Pausa entre pruebas
  bool resultBA = testSequence(gpioB, gpioA);
  if (!resultBA)
    return false;
  return true;  // Ambas pruebas correctas
}

bool testSequence(uint8_t gpioOut, uint8_t gpioIn) {

  pinMode(gpioOut, OUTPUT);
  pinMode(gpioIn, INPUT);  // Configuración con resistencia
  uint8_t testPattern[] = { 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1 };

  for (int i = 0; i < sizeof(testPattern); i++) {
    digitalWrite(gpioOut, testPattern[i]);  // Envía bit
    delay(10);                              // Espera de estabilización
    int readValue = digitalRead(gpioIn);    // Lee bit
    if (readValue != testPattern[i]) {
      return false;
    }
  }

  return true;  // Todos los bits coincidieron correctamente
}

bool i2cCheckDevice(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);
}

void demo() {
  int delay_ms = 100;
  int neop = 1;
  digitalWrite(SDA_PIN, HIGH);
  delay(delay_ms);

  // ---- Neopixel en Rojo ----
  for (int i = 0; i < neop; i++) {
    pixels.setPixelColor(i, pixels.Color(intensity, 0, 0));
    pixels.show();
    delay(delay_ms);
  }

  digitalWrite(SDA_PIN, LOW);
  delay(delay_ms);

  // ---- Neopixel en Verde ----

  for (int i = 0; i < neop; i++) {
    pixels.setPixelColor(i, pixels.Color(0, intensity, 0));
    pixels.show();
    delay(delay_ms);
  }

  digitalWrite(SDA_PIN, HIGH);
  delay(delay_ms);

  // ---- Neopixel en Azul----

  for (int i = 0; i < neop; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, intensity));
    pixels.show();
    delay(delay_ms);
  }

  pixels.clear();
  digitalWrite(SDA_PIN, LOW);
  delay(delay_ms);
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {

  Serial.printf("Listing directory: %s\n", dirname);
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }

  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();

  while (file) {

    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {

        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }

    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {

  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {

  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {

  Serial.printf("Reading file: %s\n", path);
  File file = fs.open(path);

  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");

  while (file.available()) {
    Serial.write(file.read());
  }

  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {

  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);

  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }

  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {

  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }

  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }

    end = millis() - start;
    Serial.printf("%u bytes read for %lu ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();

  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }

  end = millis() - start;
  Serial.printf("%u bytes written for %lu ms\n", 2048 * 512, end);
  file.close();
}
