/**
 * PULSAR H2 - Bluetooth Web Control Firmware
 * 
 * Firmware para ESP32 Pulsar H2 que integra:
 * - Control Bluetooth Low Energy
 * - NeoPixel RGB
 * - microSD SPI
 * - ADC GPIO2/GPIO3
 * - GPIO9 entrada
 * 
 * Desarrollado por UNIT Electronics
 * Compatible con Pulsar H2
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_DRV2605.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// ========================================
// CONFIGURACIÓN HARDWARE PULSAR H2
// ========================================

// ---------- NeoPixel ----------
#define RGB_LED_PIN 8
#define NUM_LEDS 1
Adafruit_NeoPixel strip(NUM_LEDS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

// ---------- Estado Controlador I2C ----------
#define SDA_PIN 12
#define SCL_PIN 22
Adafruit_DRV2605 drv;
bool drvConnected = false;

// ---------- GPIO entrada ----------
#define INPUT_PIN 9
#define LED_BUILTIN 4

// ---------- SPI + SD ----------
bool sdConectada = false;  // bandera global
#define MOSI_PIN 5
#define MISO_PIN 0
#define SCK_PIN 4
#define CS_PIN 11
File myFile;

// ---------- JSON de salida ----------
String JSON_out;
StaticJsonDocument<200> sendJSON;
char buffer[256];

// ---------- ADC ----------
const int adcPin_A2 = 2;  // GPIO2 (A2)
const int adcPin_A3 = 3;  // GPIO3 (A3)

// ========================================
// UUIDs BLE - Deben coincidir con la web
// ========================================
#define SERVICE_UUID "12345678-1234-1234-1234-1234567890ab"
#define NEOPIXEL_CHARACTERISTIC_UUID "12345678-1234-1234-1234-1234567890ac"
#define SENSOR_CHARACTERISTIC_UUID "12345678-1234-1234-1234-1234567890ad"
#define GPIO_CHARACTERISTIC_UUID "12345678-1234-1234-1234-1234567890ae"
#define SD_CHARACTERISTIC_UUID "12345678-1234-1234-1234-1234567890af"

// ========================================
// VARIABLES GLOBALES BLE
// ========================================
BLEServer* pServer = NULL;
BLECharacteristic* pNeopixelCharacteristic = NULL;
BLECharacteristic* pSensorCharacteristic = NULL;
BLECharacteristic* pGpioCharacteristic = NULL;
BLECharacteristic* pSdCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
int lastGpioState = -1;
unsigned long lastSensorRead = 0;
unsigned long lastDisplayUpdate = 0;
const unsigned long SENSOR_INTERVAL = 1000;  // Leer sensores cada 1 segundo
const unsigned long DISPLAY_INTERVAL = 500;  // Actualizar display cada 500ms
bool deviceInfoSent = false;                 // Flag para enviar Device Info solo una vez al conectar
// ========================================
// DECLARACIONES ADELANTADAS
// ========================================
// ========================================
// DECLARACIONES ADELANTADAS
// ========================================
void ledColor(uint8_t r, uint8_t g, uint8_t b);  // ← NECESARIO

// Variables para el hardware
bool sdCardOK = false;
uint8_t currentR = 0, currentG = 0, currentB = 0;



// ========================================
// FORWARD DECLARATIONS
// ========================================
String getDeviceInfo();

// ========================================
// CALLBACKS BLE
// ========================================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    deviceInfoSent = false;  // Reset flag al conectar
    Serial.println("Cliente conectado");

    // *** ENVIAR ESTADO INICIAL AL CONECTAR ***
    delay(1500);  // Dar más tiempo para que el cliente configure notificaciones

    // Enviar estado actual de GPIO9
    if (pGpioCharacteristic) {
      String currentGpioState = String(digitalRead(INPUT_PIN));
      pGpioCharacteristic->setValue(currentGpioState.c_str());
      pGpioCharacteristic->notify();
      Serial.printf("� Estado inicial GPIO9 enviado: %s\n", currentGpioState.c_str());
    }

    // Pequeña pausa antes de enviar Device Info
    delay(300);

    // Enviar información del dispositivo via característica Sensor
    if (pSensorCharacteristic) {
      String deviceInfo = getDeviceInfo();
      // Formato: DEVICE_INFO:{json}
      String infoMessage = "DEVICE_INFO:" + deviceInfo;
      pSensorCharacteristic->setValue(infoMessage.c_str());
      pSensorCharacteristic->notify();
      deviceInfoSent = true;
      Serial.println("� Información del dispositivo enviada:");
      Serial.println(deviceInfo);
    }

    // Pequeña pausa antes de enviar datos de sensores
    delay(500);

    // Enviar estado inicial de sensores
    if (pSensorCharacteristic) {
      int value_A2 = analogRead(adcPin_A2);
      int value_A3 = analogRead(adcPin_A3);
      float voltage_A2 = (value_A2 / 4095.0) * 3.3;
      float voltage_A3 = (value_A3 / 4095.0) * 3.3;

      String sensorData = "a2:" + String(voltage_A2, 3) + ",a3:" + String(voltage_A3, 3);
      pSensorCharacteristic->setValue(sensorData.c_str());
      pSensorCharacteristic->notify();
      Serial.println("🔧 Estado inicial sensores enviado");
    }
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    deviceInfoSent = false;  // Reset flag al desconectar
    Serial.println("Cliente desconectado");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string rxValue = std::string(pCharacteristic->getValue().c_str());

    if (rxValue.length() > 0) {
      Serial.print("Comando BLE recibido: ");
      Serial.println(rxValue.c_str());

      // Control del NeoPixel
      if (pCharacteristic->getUUID().toString() == NEOPIXEL_CHARACTERISTIC_UUID) {
        // Formato: "r,g,b" ejemplo: "255,0,0" para rojo
        int r, g, b;
        if (sscanf(rxValue.c_str(), "%d,%d,%d", &r, &g, &b) == 3) {
          r = constrain(r, 0, 255);
          g = constrain(g, 0, 255);
          b = constrain(b, 0, 255);
          ledColor(r, g, b);
          currentR = r;
          currentG = g;
          currentB = b;
          Serial.printf("NeoPixel: R=%d G=%d B=%d\n", r, g, b);
        }
      }
    }
  }
};

// ========================================
// FUNCIONES AUXILIARES HARDWARE
// ========================================
void ledColor(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

// Función para obtener información del dispositivo
String getDeviceInfo() {
  // Obtener MAC Address (para ESP32-H2 usamos la MAC de la EFUSE)
  uint64_t efuseMac = ESP.getEfuseMac();
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          (uint8_t)(efuseMac >> 0), (uint8_t)(efuseMac >> 8),
          (uint8_t)(efuseMac >> 16), (uint8_t)(efuseMac >> 24),
          (uint8_t)(efuseMac >> 32), (uint8_t)(efuseMac >> 40));
  String macAddress = String(macStr);

  // Obtener Chip ID (único para cada ESP32)
  uint64_t chipID = ESP.getEfuseMac();

  // Convertir Chip ID a string hexadecimal
  char chipIDStr[17];
  sprintf(chipIDStr, "%04X%08X", (uint16_t)(chipID >> 32), (uint32_t)chipID);

  // Obtener información adicional
  String chipModel = ESP.getChipModel();
  uint8_t chipRevision = ESP.getChipRevision();
  uint32_t chipCores = ESP.getChipCores();
  uint32_t flashSize = ESP.getFlashChipSize();

  // Crear String JSON con toda la información
  String deviceInfo = macAddress + "//"
                      + String(chipIDStr);

  return deviceInfo;
}



// Función para diagnóstico del GPIO9
bool diagnoseGPIO9() {

  bool status = false;

  Serial.println("\n🔍 === DIAGNÓSTICO GPIO9 ===");

  // Información de configuración
  Serial.printf("📌 Pin configurado: GPIO%d\n", INPUT_PIN);
  Serial.println("📌 Modo: INPUT_PULLUP");
  Serial.println("📌 Pull-up interno: ACTIVADO");

  // Lecturas múltiples para estabilidad
  Serial.println("📊 Realizando 10 lecturas consecutivas:");
  int highCount = 0, lowCount = 0;

  for (int i = 0; i < 10; i++) {
    int reading = digitalRead(INPUT_PIN);
    Serial.printf("   Lectura %2d: %d (%s)\n", i + 1, reading, reading == HIGH ? "HIGH" : "LOW");

    if (reading == HIGH) highCount++;
    else lowCount++;

    delay(100);
  }

  // Análisis de estabilidad
  Serial.printf("📈 Resumen: %d lecturas HIGH, %d lecturas LOW\n", highCount, lowCount);

  if (highCount == 10) {
    Serial.println("✅ GPIO9 estable en HIGH (sin conexión a tierra)");
    status = true;
  } else if (lowCount == 10) {
    Serial.println("✅ GPIO9 estable en LOW (conectado a tierra)");
    status = false;
  } else {
    Serial.println("⚠️ GPIO9 presenta lecturas inconsistentes - posible ruido");
  }

  // Estado actual vs inicial
  int currentState = digitalRead(INPUT_PIN);
  Serial.printf("🎯 Estado actual: %s\n", currentState == HIGH ? "HIGH" : "LOW");
  Serial.printf("🎯 Estado inicial: %s\n", lastGpioState == HIGH ? "HIGH" : "LOW");

  if (currentState == lastGpioState) {
    Serial.println("✅ Estado consistente desde el inicio");
  } else {
    Serial.println("🔄 Estado cambió desde el inicio");
  }

  Serial.println("=== FIN DIAGNÓSTICO ===\n");
  return status;
}


// ========================================
// CONFIGURACIÓN INICIAL
// ========================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== UNIT ELECTRONICS PULSAR H2 BLUETOOTH CONTROL ===");

  // --- NeoPixel arranque ---
  strip.begin();
  strip.setBrightness(30);
  strip.show();
  ledColor(0, 0, 255);  // Azul durante inicialización
  delay(500);

  // --- SPI + microSD ---
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  Serial.println("\nInicializando microSD...");
  if (!SD.begin(CS_PIN)) {
    Serial.println("❌ Error al inicializar microSD");
    sdCardOK = false;
  } else {
    Serial.println("✅ microSD OK");
    sdCardOK = true;

    // Crear archivo de log inicial
    myFile = SD.open("/bluetooth_log.txt", FILE_WRITE);
    if (myFile) {
      myFile.println("=== PULSAR H2 BLE LOG INICIADO ===");
      myFile.close();
    }
  }


  // --- Configurar GPIO9 ---
  pinMode(INPUT_PIN, INPUT_PULLUP);
  Serial.println("GPIO9 configurado (INPUT_PULLUP)");
  pinMode(LED_BUILTIN, OUTPUT);


  // --- Configurar Haptic I2C ---
  Wire.beginTransmission(0x5A);
  bool found = (Wire.endTransmission() == 0);

  if (found) {
    if (!drvConnected) {
      Serial.println("DRV2605L detectado. Reconfigurando bus...");

      // Reiniciar bus I2C por seguridad
      Wire.end();
      delay(50);
      Wire.begin(SDA_PIN, SCL_PIN);
      delay(50);

      // Reiniciar e inicializar chip
      if (drv.begin(&Wire)) {
        drv.selectLibrary(1);
        drv.useERM();
        drv.setMode(DRV2605_MODE_INTTRIG);
        drvConnected = true;
        Serial.println("DRV2605L inicializado correctamente.");
      } else {
        Serial.println("Error al inicializar DRV2605L.");
        drvConnected = false;
      }
    }

    if (drvConnected) {
      Serial.println("Modo 1...");
      hapticMode1();
    }

  } else {
    if (drvConnected) {
      Serial.println("DRV2605L desconectado.");
      drvConnected = false;
    }
  }


  // *** LECTURA INICIAL DEL GPIO9 ***
  /*

  delay(100);  // Pequeña pausa para estabilizar la lectura
  int initialGpioState = digitalRead(INPUT_PIN);
  lastGpioState = initialGpioState;  // Establecer estado inicial

  Serial.print("🔍 Estado inicial GPIO9: ");
  Serial.println(initialGpioState == HIGH ? "HIGH" : "LOW");
  Serial.printf("🔍 Valor digital leído: %d\n", initialGpioState);

  // Validar configuración con múltiples lecturas
  Serial.println("🔍 Validando configuración GPIO9...");
  for (int i = 0; i < 5; i++) {
    int reading = digitalRead(INPUT_PIN);
    Serial.printf("   Lectura %d: %d (%s)\n", i + 1, reading, reading == HIGH ? "HIGH" : "LOW");
    delay(50);
  }

  // Confirmar estado estable
  int finalReading = digitalRead(INPUT_PIN);
  if (finalReading == initialGpioState) {
    Serial.println("✅ GPIO9 configuración estable confirmada");
  } else {
    Serial.println("⚠️ GPIO9 presenta lecturas inconsistentes");
    lastGpioState = finalReading;  // Usar la última lectura
  }


  Serial.printf("🎯 Estado inicial confirmado GPIO9: %s\n",
                lastGpioState == HIGH ? "HIGH" : "LOW");

  */

  // --- Configurar ADC ---
  analogReadResolution(12);
  Serial.println("ADC habilitado: A2(GPIO2) y A3(GPIO3)");

  // --- Configurar BLE ---
  setupBLE();

  // Realizar diagnóstico completo del GPIO9
  diagnoseGPIO9();

  // Indicar listo
  ledColor(0, 255, 0);  // Verde = listo

  Serial.println("Pulsar H2 listo para conexiones Bluetooth");
  Serial.println("Nombre del dispositivo: Pulsar_H2");

  // *** RESUMEN DEL ESTADO INICIAL ***
  Serial.println("\n🎯 === ESTADO INICIAL DEL SISTEMA ===");
  Serial.printf("🔧 GPIO9: %s (valor: %d)\n", lastGpioState == HIGH ? "HIGH" : "LOW", lastGpioState);
  //Serial.printf("💾 MicroSD: %s\n", sdCardOK ? "OK" : "ERROR");
  Serial.printf("📡 BLE: INICIADO\n");
  Serial.printf("🎨 NeoPixel: RGB(0,255,0) - VERDE\n");
  Serial.printf("⏱️ Tiempo de inicialización: %lu ms\n", millis());
  Serial.println("=== SISTEMA LISTO ===\n");
}

// ========================================
// CONFIGURACIÓN BLE
// ========================================
void setupBLE() {
  // Crear dispositivo BLE
  BLEDevice::init("Pulsar_H2");

  // Crear servidor BLE
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Crear servicio BLE
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Crear característica para NeoPixel
  pNeopixelCharacteristic = pService->createCharacteristic(
    NEOPIXEL_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pNeopixelCharacteristic->setCallbacks(new MyCallbacks());

  // Crear característica para sensores (ADC + GPIO)
  pSensorCharacteristic = pService->createCharacteristic(
    SENSOR_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  // Crear característica para GPIO
  pGpioCharacteristic = pService->createCharacteristic(
    GPIO_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  // Crear característica para SD
  pSdCharacteristic = pService->createCharacteristic(
    SD_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pSdCharacteristic->setCallbacks(new MyCallbacks());

  // Agregar descriptores BLE2902 solo para las características críticas
  // ESP32-H2 tiene limitaciones: máximo 4 características activas
  // Priorizamos las notificaciones más importantes
  pSensorCharacteristic->addDescriptor(new BLE2902());  // Crítico: lecturas continuas + device info
  pGpioCharacteristic->addDescriptor(new BLE2902());    // Crítico: cambios de estado

  // Inicializar valores con estado real del GPIO9
  pNeopixelCharacteristic->setValue("0,255,0");
  pSensorCharacteristic->setValue("a2:0.0,a3:0.0");

  // Establecer el valor inicial real del GPIO9
  String initialGpioValue = String(lastGpioState);
  pGpioCharacteristic->setValue(initialGpioValue.c_str());
  Serial.printf("🔧 BLE GPIO característica inicializada con: %s\n", initialGpioValue.c_str());

  pSdCharacteristic->setValue(sdCardOK ? "OK" : "ERROR");

  Serial.println("🔧 Device Info se enviará via característica Sensor al conectar");

  // Iniciar servicio
  pService->start();

  // Iniciar advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();

  Serial.println("Servicio BLE iniciado, esperando conexiones...");
}

// ========================================
// LOOP PRINCIPAL
// ========================================
void loop() {

  //runNeopixel();

  //bool status = verificarSD();
  //Serial.print("Inicialización de la SD: ");
  //Serial.println(sdCardOK);

  hapticMode1();

  // Manejar conexión/desconexión BLE
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Reiniciando advertising BLE");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }


  // Leer sensores y GPIO
  int gpioState = digitalRead(INPUT_PIN);
  int value_A2 = analogRead(adcPin_A2);
  int value_A3 = analogRead(adcPin_A3);
  float voltage_A2 = (value_A2 / 4095.0) * 3.3;
  float voltage_A3 = (value_A3 / 4095.0) * 3.3;

  // Enviar datos via BLE si está conectado
  if (deviceConnected && millis() - lastSensorRead > SENSOR_INTERVAL) {
    // Si aún no se ha confirmado el envío de Device Info, reenviarlo cada 5 segundos
    static unsigned long lastDeviceInfoRetry = 0;
    if (!deviceInfoSent || (millis() - lastDeviceInfoRetry > 5000)) {
      String deviceInfo = getDeviceInfo();
      String infoMessage = "DEVICE_INFO:" + deviceInfo;
      pSensorCharacteristic->setValue(infoMessage.c_str());
      pSensorCharacteristic->notify();
      lastDeviceInfoRetry = millis();
      Serial.println("🔄 Reenviando Device Info...");
      delay(200);  // Pausa para que se procese
    }

    //String infoMessage = "DEVICE_INFO:" + deviceInfo;

    // === Envío de la MAC y ChipID ====
    String data = getDeviceInfo();
    int sepIndex = data.indexOf("//");
    String mac = data.substring(0, sepIndex);
    String IDchip = data.substring(sepIndex + 2);

    sendJSON["mac"] = String(mac);
    sendJSON["idchip"] = String(IDchip);
    //serializeJson(sendJSON, buffer);
    ////pSensorCharacteristic->setValue(sensorData.c_str());
    //pSensorCharacteristic->setValue(buffer);
    //pSensorCharacteristic->notify();
    lastDeviceInfoRetry = millis();
    Serial.println("🔄 Reenviando Device Info...");
    delay(200);  // Pausa para que se procese


    // === Validación de puertos analógicos ====
    // Enviar datos de sensores ADC
    String sensorData = "a2:" + String(voltage_A2, 3) + ",a3:" + String(voltage_A3, 3);

    if (voltage_A2 > 0 && voltage_A3 > 0) {
      sendJSON["analog"] = "OK";
    } else {
      sendJSON["analog"] = "F";
    }

    if (sdCardOK) {
      sendJSON["sd"] = "OK";
    } else {
      sendJSON["sd"] = "F";
    }

    delay(500);
    if (drvConnected) {
      sendJSON["i2c"] = "OK";
    } else {
      sendJSON["i2c"] = "F";
    }

    bool stGpio = diagnoseGPIO9();
    if (stGpio) {
      sendJSON["gpio"] = "OK";
    } else {
      sendJSON["gpio"] = "F";
    }

    sendJSON["Lectura"] = sensorData;
    serializeJson(sendJSON, buffer);
    //pSensorCharacteristic->setValue(sensorData.c_str());
    pSensorCharacteristic->setValue(buffer);
    pSensorCharacteristic->notify();

    lastSensorRead = millis();
  }

  delay(50);
}





bool verificarSD() {
  File testFile;
  bool sDConectada;

  // Intentar abrir un archivo de prueba
  testFile = SD.open("/test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("Ping SD");
    testFile.close();

    if (!sdConectada) {
      Serial.println("✅ microSD conectada");
      sdConectada = true;
    }
  } else {
    if (sdConectada) {
      Serial.println("❌ microSD desconectada");
      sdConectada = false;
    }
  }

  return sdConectada;
}


void runNeopixel() {

  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);

  ledColor(245, 41, 109);
  delay(20);
  ledColor(0, 0, 0);
  delay(20);

  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);

  ledColor(245, 41, 109);
  delay(20);
  ledColor(0, 0, 0);
  delay(20);

  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);

  ledColor(245, 41, 109);
  delay(20);
  ledColor(0, 0, 0);
  delay(20);

  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);

  ledColor(255, 0, 0);
  delay(200);

  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);

  ledColor(255, 128, 0);
  delay(200);

  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);
  ledColor(0, 255, 0);
  delay(200);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);
  ledColor(0, 255, 255);
  delay(200);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);
  ledColor(0, 0, 255);
  delay(200);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);
  ledColor(128, 0, 255);
  delay(200);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);
  ledColor(255, 0, 0);
  delay(200);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);
  ledColor(255, 0, 255);
  delay(200);
  ledColor(0, 0, 0);
}

void hapticMode1() {
  drv.setWaveform(0, 118);  // vibración fuerte
  drv.setWaveform(1, 0);    // fin
  drv.go();
  delay(100);
}