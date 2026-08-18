#include "BLEDevice.h"
#include <ArduinoJson.h>

static BLEUUID serviceUUID("12345678-1234-5678-1234-56789abcdef0");
static BLEUUID charUUID("abcdef01-1234-5678-1234-56789abcdef0");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// Callback para detectar el Servidor durante el escaneo
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
      Serial.println("¡Servidor PulsarC6 encontrado!");
    }
  }
};

bool connectToServer() {
  Serial.println("Conectando al Servidor...");
  BLEClient* pClient = BLEDevice::createClient();
  pClient->connect(myDevice);

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    pClient->disconnect();
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    pClient->disconnect();
    return false;
  }

  connected = true;
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando PulsarC6 BLE Client...");
  BLEDevice::init("PulsarC6-Client");

  // Iniciar escaneo para buscar el servidor
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  // Conectar si se encontró el dispositivo
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("Conectado exitosamente.");
    } else {
      Serial.println("Fallo al conectar.");
    }
    doConnect = false;
  }

  // Si estamos conectados, enviamos el JSON
  if (connected) {
    // 1. Simular lectura de sensores
    float temperaturaActual = random(2000, 3000) / 100.0;  // Ej: 25.40
    int humedadActual = random(40, 80);                    // Ej: 60

    // 2. Construir el JSON
    StaticJsonDocument<200> doc;
    doc["id_dispositivo"] = "Sensor-Norte";
    doc["temperatura"] = temperaturaActual;
    doc["humedad"] = humedadActual;

    String jsonString;
    serializeJson(doc, jsonString);

    // 3. Escribir el JSON en la característica del servidor
    Serial.println("Enviando JSON: " + jsonString);
    pRemoteCharacteristic->writeValue(jsonString.c_str(), jsonString.length());

  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // Reiniciar escaneo si perdimos conexión
  }

  delay(5000);  // Enviar datos cada 5 segundos
}