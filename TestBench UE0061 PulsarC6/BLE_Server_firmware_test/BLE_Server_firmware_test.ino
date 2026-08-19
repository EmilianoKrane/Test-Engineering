/*
Este firmware es una adaptación del ejemplo BLE_Server_example contenido en el mismo 
directorio, con las claves de JSON que entrega el firmware de prueba flasheado en las pulsarc6
para la revisión de sus perifericos. 

Entrega en formato JSON los resultados obtenidos para su registro y validación en el frontend 
de pruebas
*/


#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ArduinoJson.h>  // Necesario para parsear el JSON recibido

// Define UUIDs únicos para tu aplicación PulsarC6
#define SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcdef01-1234-5678-1234-56789abcdef0"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

// Callbacks para gestionar la conexión
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("¡Cliente conectado!");
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Cliente desconectado. Reiniciando anuncio...");
    BLEDevice::startAdvertising();  // Vuelve a anunciarse para el próximo cliente
  }
};

// Callbacks para gestionar cuando el cliente escribe el JSON
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String rxValue = pChar->getValue();

    if (rxValue.length() > 0) {
      Serial.println("--- Paquete JSON Recibido ---");
      Serial.println(rxValue);

      // Opcional: Parsear el JSON recibido
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, rxValue);

      if (!error) {
        bool i2c = doc["i2c_bus"];
        bool spi = doc["spi_bus"];
        bool wifi = doc["WiFi_status"];
        String mac = doc["mac"];

        Serial.print("Parseado OK -> i2c:" + String(i2c) + " spi: " + String(spi) + " wifi: " + String(wifi) + " mac: " + mac + "\n");
      } else {
        Serial.println("Error al parsear el JSON");
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando PulsarC6 BLE Server...");

  BLEDevice::init("PulsarC6-Server");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Creamos la característica con permisos para ser escrita por el cliente
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pService->start();

  // Iniciar el anuncio (Advertising)
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("Servidor listo. Esperando clientes...");
}

void loop() {
  // El servidor maneja las recepciones de forma asíncrona mediante el callback onWrite.
  // Aquí puedes poner el código no bloqueante de tu nodo central.
  delay(2000);
}