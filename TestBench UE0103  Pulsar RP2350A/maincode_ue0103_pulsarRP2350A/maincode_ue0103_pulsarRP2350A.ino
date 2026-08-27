/*
Firmware Test blink pulsar rp2350a  controlado desde uart externo
*/


// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SDFS.h>
#include <udvi_hstx.h>
#include <PDM.h>
#include "SparkFun_BMI270_Arduino_Library.h"

// ============ CONFIGURACIÓN HDMI ============
DVHSTXPinout pinConfig = { 14, 18, 16, 12 };
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// ============ CONFIGURACIÓN BMI270 ============
BMI270 imu;
uint8_t i2cAddress = BMI2_I2C_PRIM_ADDR + 1;  // 0x69
#define SDA_PIN 8                             // >> GPIO08 SDA I2C Conectado a IMU BMI270
#define SCL_PIN 9                             // >> GPIO09 SCL I2C Conectado a IMU BMI270

// ============ CONFIGURACIÓN MICRO SD ============
#define SD_MOSI_PIN 3  // SDIO_CMD
#define SD_MISO_PIN 4  // SDIO_DAT0
#define SD_SCK_PIN 2   // SDIO_CLK
#define SD_CS_PIN 7    // SDIO_DAT3

// ============ CONFIGURACIÓN PSRAM ============
#if defined(RP2350_PSRAM_CS)
#define PSRAM_TEST_SIZE 64                     // Solo 64 bytes - mínimo
uint8_t psramBuffer[1024] PSRAM;               // Solo 1KB en PSRAM (reducido de 256KB)
uint8_t psramTestData[PSRAM_TEST_SIZE] PSRAM;  // Test data también en PSRAM
#endif

// ============ CONFIGURACIÓN PDM MICROPHONE ============
static const int kSampleRate = 8000;
static const int kChannels = 1;
static const size_t kSampleBufferCount = 128;  // Reducido a 128 para ahorrar RAM
static const int kPdmDinPin = 11;
static const int kPdmClkPin = 10;

static int16_t sampleBuffer[kSampleBufferCount];
static volatile size_t samplesRead = 0;
static volatile int audioLevelRMS = 0;  // Nivel RMS del audio (0-32767)
bool pdmAvailable = false;

// Buffer de historial de audio para gráfica (últimos 64 valores RMS)
#define AUDIO_HISTORY_SIZE 64
int audioHistory[AUDIO_HISTORY_SIZE];
int audioHistoryIndex = 0;

// ============ CONFIGURACIÓN PINES DE LEDS ============
#define WS_PIN 1        // >> GPIO DE NEOPIXEL
#define LED_BUILTIN 22  // >> LED BUILTIN
#define BUTTON_PIN 24   // >> PIN DE ENTRADA CON BOTONERA EN QWIIC

// ============ VARIABLES DEL CUBO 3D ============
float vertices[8][3] = {
  { -1, -1, -1 }, { 1, -1, -1 }, { 1, 1, -1 }, { -1, 1, -1 }, { -1, -1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { -1, 1, 1 }
};

int edges[12][2] = {
  { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },  // Cara frontal
  { 4, 5 },
  { 5, 6 },
  { 6, 7 },
  { 7, 4 },  // Cara trasera
  { 0, 4 },
  { 1, 5 },
  { 2, 6 },
  { 3, 7 }  // Conexiones
};

float angleX = 0, angleY = 0, angleZ = 0;
int centerX, centerY;
float scale = 30;  // Reducido para dejar espacio

// ============ ESTADO DEL SISTEMA ============
bool imuControl = true;    // true = IMU, false = auto
bool sdAvailable = false;  // Estado de la SD
int fileCount = 0;         // Archivos en SD
unsigned long lastLogTime = 0;
unsigned long logInterval = 5000;  // Log cada 5 segundos

// Estado PSRAM
bool psramAvailable = false;
int psramSizeMB = 0;
int psramFreeKB = 0;
unsigned long lastPSRAMTest = 0;
unsigned long psramTestInterval = 60000;  // Test cada 60 segundos (muy reducido)
int psramTestsPassed = 0;
bool enablePSRAMTests = false;  // Deshabilitar tests por defecto para ahorrar RAM

// Heartbeat y diagnóstico
unsigned long frameCount = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long displayUpdateInterval = 100;  // Actualizar display cada 100ms (10 FPS)
int lastFreeHeap = 0;

// ==== DECLARACIÓN DE VARIABLES GLOBALES y MACROS ====
#define NUM_NEOP 3         // Cantidad de Neopixeles a controlar
#define OLED_RESET -1      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128   // OLED display width, in pixels
#define SCREEN_HEIGHT 64   // OLED display height, in pixels
bool status_OLED = false;  // Variable check de inicialización OLED por I2C
String JSON_entrada;       // Variable que recibe JSON del frontend
String JSON_salida;        // Variable que envía el JSON de datos

// ==== CREACIÓN DE OBJETOS ====
SerialPIO PagWeb(20, 21);                                          // Sintaxis: SerialPIO nombre(TX_PIN, RX_PIN);
Adafruit_NeoPixel pixels(NUM_NEOP, WS_PIN, NEO_GRB + NEO_KHZ800);  // Objeto de Neopixel
JsonDocument receiveJSON;
JsonDocument sendJSON;


// ============ CALLBACK PDM ============
void onPdmData() {
  const int bytesAvailable = PDM.available();
  if (bytesAvailable <= 0) {
    return;
  }

  const int bytesToRead = min(bytesAvailable, (int)sizeof(sampleBuffer));
  const int bytesRead = PDM.read((void*)sampleBuffer, bytesToRead);
  if (bytesRead > 0) {
    samplesRead = (size_t)bytesRead / sizeof(sampleBuffer[0]);
  }
}

// ==== FUNCIONES DE UTILIDAD ====
void pagwebDebug(String cmd) {
  JsonDocument doc;
  doc["debug"] = cmd;
  serializeJson(doc, PagWeb);
  PagWeb.println();
}

void setup() {
  Serial.begin(115200);

  // Puedes dejar los Serial.println del setup porque ocurren
  // ANTES de que empiece la transmisión binaria constante.
  Serial.println("Iniciando PULSAR RP2350A...");

  // ============ INICIALIZACIÓN PDM ============
  PDM.setDIN(kPdmDinPin);
  PDM.setCLK(kPdmClkPin);
  PDM.onReceive(onPdmData);
  PDM.setBufferSize(sizeof(sampleBuffer));

  if (!PDM.begin(kChannels, kSampleRate)) {
    Serial.println("Error: PDM microphone");
  }

  // ============ INICIALIZACIÓN HDMI ============
  if (!display.begin()) {
    Serial.println("Error: Display HDMI");
  }
  centerX = display.width() / 2;
  centerY = display.height() / 2;
  display.fillScreen(0x0000);

  // ============ INICIALIZACIÓN IMU ============
  Wire.setSDA(8);
  Wire.setSCL(9);
  Wire.begin();
  if (imu.beginI2C(i2cAddress) == BMI2_OK) {
    imuControl = true;
  } else {
    imuControl = false;
  }

  Serial.println("Setup Core 0 completado. Iniciando stream de audio...");
}

void loop() {
  // 1. TRANSMISIÓN DE AUDIO (Prioridad Alta)
  // Revisa si hay muestras listas y las envía inmediatamente
  if (samplesRead > 0) {
    noInterrupts();
    size_t localCount = samplesRead;
    samplesRead = 0;
    interrupts();

    if (localCount > kSampleBufferCount) {
      localCount = kSampleBufferCount;
    }

    // Enviar TODO el buffer en binario
    Serial.write((uint8_t*)sampleBuffer, localCount * sizeof(int16_t));
  }

  // 2. LECTURA DEL SENSOR IMU
  if (imuControl) {
    // Modo IMU: Leer sensor
    imu.getSensorData();

    angleX += imu.data.gyroX * 0.3;
    angleY += imu.data.gyroY * 0.3;
    angleZ += imu.data.gyroZ * 0.3;

    // Mantener en rango
    angleX = fmod(angleX, 360);
    angleY = fmod(angleY, 360);
    angleZ = fmod(angleZ, 360);
  } else {
    // Modo auto
    angleX += 1.5;
    angleY += 2.0;
    angleZ += 1.0;

    if (angleX >= 360) angleX -= 360;
    if (angleY >= 360) angleY -= 360;
    if (angleZ >= 360) angleZ -= 360;
  }

  // 3. ACTUALIZACIÓN DE PANTALLA HDMI (No bloqueante)
  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {

    // Limpiamos el área del cubo
    display.fillRect(60, 60, 200, 120, 0x0000);

    // Dibujar cubo 3D (usará anguloX, anguloY, anguloZ)
    drawCube();

    // ============ MOSTRAR INFO SUPERIOR ============
    // Limpiar barra superior para evitar solapamiento de texto
    display.fillRect(0, 0, 320, 15, 0x0000);

    display.setTextSize(1);
    display.setCursor(5, 5);

    if (imuControl) {
      display.setTextColor(0x07E0);  // Verde
      display.print("IMU: OK");
    } else {
      display.setTextColor(0xF800);  // Rojo
      display.print("IMU: AUTO");
    }

    // Estado SD
    display.setCursor(80, 5);
    if (sdAvailable) {
      display.setTextColor(0x07E0);  // Verde
      display.print("SD: ");
      display.print(fileCount);
      display.print(" files");
    } else {
      display.setTextColor(0xF800);  // Rojo
      display.print("SD: N/A");
    }

    // Estado PSRAM
    display.setCursor(200, 5);
    if (psramAvailable) {
      display.setTextColor(0x07E0);  // Verde
      display.print("RAM:");
      display.print(psramSizeMB);
      display.print("M");
    } else {
      display.setTextColor(0xF800);  // Rojo
      display.print("RAM:N/A");
    }

    // Estado PDM Microphone
    display.setCursor(260, 5);
    if (pdmAvailable) {
      display.setTextColor(0x07E0);  // Verde
      display.print("MIC");
    } else {
      display.setTextColor(0xF800);  // Rojo
      display.print("---");
    }

    // Barra de nivel de audio (si está disponible)
    if (pdmAvailable && audioLevelRMS > 0) {
      // Mapear RMS a ancho de barra (0-60 píxeles)
      int barWidth = map(audioLevelRMS, 0, 10000, 0, 60);
      barWidth = constrain(barWidth, 0, 60);

      // Dibujar barra de audio
      display.fillRect(258, 12, 60, 3, 0x0000);  // Limpiar fondo
      if (barWidth > 0) {
        uint16_t barColor = 0x07E0;                        // Verde por defecto
        if (audioLevelRMS > 7000) barColor = 0xF800;       // Rojo si está alto
        else if (audioLevelRMS > 4000) barColor = 0xFFE0;  // Amarillo medio

        display.fillRect(258, 12, barWidth, 3, barColor);
      }
    }

    // Tests PSRAM pasados (pequeño contador)
    if (psramAvailable && psramTestsPassed > 0) {
      display.setCursor(300, 5);
      display.setTextColor(0xFFE0);  // Amarillo
      display.print(psramTestsPassed);
    }

    // ============ MOSTRAR DATOS SENSOR ============
    // Limpiar barra inferior (solo la parte izquierda, no tocar la gráfica)
    display.fillRect(0, 215, 155, 25, 0x0000);  // Solo lado izquierdo

    display.setCursor(5, 220);
    display.setTextColor(0xFFE0);  // Amarillo
    display.setTextSize(1);

    if (imuControl) {
      display.print("G:");
      display.print((int)imu.data.gyroX);
      display.print(",");
      display.print((int)imu.data.gyroY);
      display.print(",");
      display.print((int)imu.data.gyroZ);

      display.setCursor(5, 230);
      display.print("A:");
      display.print((int)(imu.data.accelX * 100));
      display.print(",");
      display.print((int)(imu.data.accelY * 100));
      display.print(",");
      display.print((int)(imu.data.accelZ * 100));
    } else {
      display.print("Auto: ");
      display.print((int)angleX);
      display.print(" ");
      display.print((int)angleY);
      display.print(" ");
      display.print((int)angleZ);
    }

    // Dibujar gráfica de audio (esquina inferior derecha)
    if (pdmAvailable) {
      drawAudioGraph();
    }

    lastDisplayUpdate = millis();
  }
}
// ============ CORE 1: COMUNICACIÓN SERIAL ============

void setup1() {
  PagWeb.begin(115200);
}

void loop1() {

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);
    if (!error) {
      String Function = receiveJSON["Function"];
      int opc = 0;
      if (Function == "ping") opc = 1;      //c
      else if (Function == "uid") opc = 2;  // {"Function":"uid"}
      else if (Function == "ram") opc = 3;  // {"Function":"ram"}
      else if (Function == "pd") opc = 4;   // {"Function":"pd"}


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
            for (int i = 0; i < 10; i++) {
              if (samplesRead == 0) {
                delay(1);
                return;
              }

              noInterrupts();
              size_t localCount = samplesRead;
              samplesRead = 0;
              interrupts();

              if (localCount > kSampleBufferCount) {
                localCount = kSampleBufferCount;
              }

              // Enviar TODO el buffer en binario de una sola vez
              // en lugar de usar un for y Serial.println()
              Serial.write((uint8_t*)sampleBuffer, localCount * sizeof(int16_t));
              delay(500);
            }

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

  delay(1);  // Pequeño respiro de 1ms para el núcleo 1
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
  delay(500);  // Pequeña pausa para que el usuario alcance a leerlo
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

  digitalWrite(LED_BUILTIN, HIGH);
  pixels.setPixelColor(0, pixels.Color(i, 0, 0));
  pixels.setPixelColor(1, pixels.Color(0, i, 0));
  pixels.setPixelColor(2, pixels.Color(0, 0, i));
  pixels.show();
  delay(delay_ms);

  digitalWrite(LED_BUILTIN, LOW);
  pixels.setPixelColor(0, pixels.Color(0, i, 0));
  pixels.setPixelColor(1, pixels.Color(0, 0, i));
  pixels.setPixelColor(2, pixels.Color(i, 0, 0));
  pixels.show();
  delay(delay_ms);

  digitalWrite(LED_BUILTIN, HIGH);
  pixels.setPixelColor(0, pixels.Color(0, 0, i));
  pixels.setPixelColor(1, pixels.Color(i, 0, 0));
  pixels.setPixelColor(2, pixels.Color(0, i, 0));
  pixels.show();
  delay(delay_ms);

  digitalWrite(LED_BUILTIN, LOW);
  pixels.setPixelColor(0, pixels.Color(0, i, 0));
  pixels.setPixelColor(1, pixels.Color(0, 0, i));
  pixels.setPixelColor(2, pixels.Color(i, 0, 0));
  pixels.show();
  delay(delay_ms);
}


void drawCube() {
  int projected[8][2];

  // Proyectar vértices
  for (int i = 0; i < 8; i++) {
    float x = vertices[i][0];
    float y = vertices[i][1];
    float z = vertices[i][2];

    // Rotación X
    float rad = angleX * PI / 180;
    float y1 = y * cos(rad) - z * sin(rad);
    float z1 = y * sin(rad) + z * cos(rad);

    // Rotación Y
    rad = angleY * PI / 180;
    float x2 = x * cos(rad) + z1 * sin(rad);
    float z2 = -x * sin(rad) + z1 * cos(rad);

    // Rotación Z
    rad = angleZ * PI / 180;
    float x3 = x2 * cos(rad) - y1 * sin(rad);
    float y3 = x2 * sin(rad) + y1 * cos(rad);

    // Proyección
    projected[i][0] = centerX + (int)(x3 * scale);
    projected[i][1] = centerY + (int)(y3 * scale);
  }

  // Dibujar solo aristas (sin círculos para reducir carga)
  for (int i = 0; i < 12; i++) {
    int v1 = edges[i][0];
    int v2 = edges[i][1];

    uint16_t color;
    if (i < 4) color = 0xF800;       // Rojo
    else if (i < 8) color = 0x001F;  // Azul
    else color = 0x07E0;             // Verde

    display.drawLine(
      projected[v1][0], projected[v1][1],
      projected[v2][0], projected[v2][1],
      color);
  }

  // Vértices simplificados: solo puntos pequeños en vez de círculos
  for (int i = 0; i < 8; i++) {
    display.drawPixel(projected[i][0], projected[i][1], 0xFFFF);
    display.drawPixel(projected[i][0] + 1, projected[i][1], 0xFFFF);
    display.drawPixel(projected[i][0], projected[i][1] + 1, 0xFFFF);
    display.drawPixel(projected[i][0] + 1, projected[i][1] + 1, 0xFFFF);
  }
}

void drawAudioGraph() {
  // Gráfica de audio en esquina inferior derecha
  const int graphX = 160;      // Posición X inicial
  const int graphY = 180;      // Posición Y inicial
  const int graphWidth = 155;  // Ancho de la gráfica
  const int graphHeight = 30;  // Alto de la gráfica
  const int maxRMS = 5000;     // Escala máxima para normalizar

  // Fondo de la gráfica (negro con borde)
  display.drawRect(graphX, graphY, graphWidth, graphHeight, 0x528A);                  // Borde gris
  display.fillRect(graphX + 1, graphY + 1, graphWidth - 2, graphHeight - 2, 0x0000);  // Fondo negro

  // Dibujar línea base (centro)
  int centerLine = graphY + graphHeight / 2;
  for (int x = graphX; x < graphX + graphWidth; x += 4) {
    display.drawPixel(x, centerLine, 0x2104);  // Línea punteada gris oscuro
  }

  // Dibujar historial de audio (últimos 64 valores)
  int samplesPerPixel = AUDIO_HISTORY_SIZE / (graphWidth - 4);
  if (samplesPerPixel < 1) samplesPerPixel = 1;

  for (int x = 0; x < graphWidth - 4; x++) {
    // Calcular índice en el historial circular
    int histIndex = (audioHistoryIndex + x * samplesPerPixel) % AUDIO_HISTORY_SIZE;
    int value = audioHistory[histIndex];

    // Normalizar a altura de gráfica
    int normalizedHeight = map(value, 0, maxRMS, 0, (graphHeight - 4) / 2);
    normalizedHeight = constrain(normalizedHeight, 0, (graphHeight - 4) / 2);

    // Calcular color según nivel (gradiente verde -> amarillo -> rojo)
    uint16_t color;
    if (value < 1500) {
      color = 0x07E0;  // Verde
    } else if (value < 3500) {
      color = 0xFFE0;  // Amarillo
    } else {
      color = 0xF800;  // Rojo
    }

    // Dibujar barra vertical desde centro
    int barX = graphX + 2 + x;
    if (normalizedHeight > 0) {
      // Dibujar hacia arriba y abajo desde el centro
      display.drawFastVLine(barX, centerLine - normalizedHeight, normalizedHeight, color);
      display.drawFastVLine(barX, centerLine, normalizedHeight, color);
    } else {
      // Sin señal, solo punto en el centro
      display.drawPixel(barX, centerLine, 0x2104);
    }
  }

  // Etiqueta "AUDIO"
  display.setCursor(graphX + 2, graphY - 8);
  display.setTextSize(1);
  display.setTextColor(0x07FF);  // Cyan
  display.print("AUDIO");

  // Indicador de pico (si hay señal alta)
  if (audioLevelRMS > 4000) {
    display.fillCircle(graphX + graphWidth - 5, graphY - 4, 2, 0xF800);  // LED rojo
  }
}

void countFiles() {
  Serial.println("\n--- Archivos en SD ---");
  fileCount = 0;

  File root = SDFS.open("/", "r");
  if (!root || !root.isDirectory()) {
    Serial.println("Error abriendo raíz");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    fileCount++;

    // Solo mostrar primeros 10 para ahorrar RAM
    if (fileCount <= 10) {
      Serial.print("  ");
      if (file.isDirectory()) {
        Serial.print("[DIR]  ");
      } else {
        Serial.print("[FILE] ");
      }
      Serial.print(file.name());
      Serial.print(" (");
      Serial.print(file.size());
      Serial.println(" bytes)");
    }

    file.close();
    yield();  // Dar tiempo entre archivos
    file = root.openNextFile();
  }
  root.close();

  Serial.print("Total: ");
  Serial.print(fileCount);
  Serial.println(" archivos\n");
  yield();
}

void createLogFile() {
  Serial.println("--- Creando archivo de log ---");

  File logFile = SDFS.open("/imu_log.csv", "w");
  if (logFile) {
    logFile.println("timestamp,gyroX,gyroY,gyroZ,accelX,accelY,accelZ");
    logFile.close();
    Serial.println("Archivo imu_log.csv creado");
  } else {
    Serial.println("Error creando archivo log");
  }

  // Crear archivo de audio si PDM está disponible
  if (pdmAvailable) {
    File audioFile = SDFS.open("/audio_log.csv", "w");
    if (audioFile) {
      audioFile.println("timestamp,rms_level");
      audioFile.close();
      Serial.println("Archivo audio_log.csv creado");
    }
  }
}

void logDataToSD() {
  // Log IMU data si está activo
  if (imuControl) {
    File logFile = SDFS.open("/imu_log.csv", "a");
    if (logFile) {
      logFile.print(millis());
      logFile.print(",");
      logFile.print(imu.data.gyroX);
      logFile.print(",");
      logFile.print(imu.data.gyroY);
      logFile.print(",");
      logFile.print(imu.data.gyroZ);
      logFile.print(",");
      logFile.print(imu.data.accelX);
      logFile.print(",");
      logFile.print(imu.data.accelY);
      logFile.print(",");
      logFile.println(imu.data.accelZ);
      logFile.flush();
      logFile.close();
      Serial.print(".");  // Indicador visual
    }
  }

  // Log audio data si PDM está disponible
  if (pdmAvailable && audioLevelRMS > 0) {
    File audioFile = SDFS.open("/audio_log.csv", "a");
    if (audioFile) {
      audioFile.print(millis());
      audioFile.print(",");
      audioFile.println(audioLevelRMS);
      audioFile.flush();
      audioFile.close();
      Serial.print("a");  // Indicador de audio
    }
  }

  yield();  // Dar tiempo al sistema después de SD
}

// ============ FUNCIONES PSRAM ============
#if defined(RP2350_PSRAM_CS)
bool testPSRAM() {
  // Test ULTRA rápido: solo 64 bytes con patrón simple
  // Generar patrón directo en PSRAM
  for (int i = 0; i < PSRAM_TEST_SIZE; i++) {
    psramTestData[i] = (uint8_t)(i & 0xFF);
  }

  // Copiar a buffer PSRAM
  memcpy(psramBuffer, psramTestData, PSRAM_TEST_SIZE);

  // Verificar solo cada 8 bytes (ultra reducido)
  for (int i = 0; i < PSRAM_TEST_SIZE; i += 8) {
    if (psramBuffer[i] != psramTestData[i]) {
      Serial.println("PSRAM test FAIL!");
      return false;
    }
  }

  // Actualizar info de heap
  psramFreeKB = rp2040.getFreePSRAMHeap() / 1024;

  return true;
}
#endif
