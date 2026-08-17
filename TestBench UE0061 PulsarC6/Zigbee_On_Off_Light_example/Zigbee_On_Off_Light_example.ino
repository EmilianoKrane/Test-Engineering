

#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"

/* Configuración del Foco Zigbee (End Device) */
#define ZIGBEE_LIGHT_ENDPOINT 10

// Pines típicos del ESP32-C6 (ajusta según tu placa Pulsarc6 si es necesario)
uint8_t led = 6;
uint8_t button = BOOT_PIN;

// Instanciamos el endpoint de la luz
ZigbeeLight zbLight = ZigbeeLight(ZIGBEE_LIGHT_ENDPOINT);

// Función de callback: Se ejecuta automáticamente cuando recibe un comando
void setLED(bool state) {
  // Enciende o apaga el LED físico dependiendo del comando recibido por RF
  digitalWrite(led, state ? HIGH : LOW);
  Serial.printf("Comando recibido: Luz %s\n", state ? "ENCENDIDA" : "APAGADA");
}

void setup() {
  Serial.begin(115200);

  // Inicializar LED físico apagado
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);

  // Inicializar el botón físico (para reset de fábrica o control local)
  pinMode(button, INPUT_PULLUP);

  // Opcional: Nombre del fabricante y modelo para identificarse en la red
  zbLight.setManufacturerAndModel("Espressif", "ZBLightBulb");

  // VINCULACIÓN: Qué función ejecutar cuando el estado de la luz cambie
  zbLight.onLightChange(setLED);

  // Registrar el endpoint en el core de Zigbee
  Serial.println("Agregando el endpoint ZigbeeLight al stack");
  Zigbee.addEndpoint(&zbLight);

  // Arrancar Zigbee (Por defecto actuará como ZIGBEE_END_DEVICE)
  if (!Zigbee.begin()) {
    Serial.println("¡Error al iniciar Zigbee!");
    Serial.println("Reiniciando...");
    ESP.restart();
  }

  Serial.println("Buscando red del Coordinador para unirse...");

  // Bucle de espera hasta encontrar la red del Switch
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n¡Conectado exitosamente a la red Zigbee!");
}

void loop() {
  // Lectura del botón físico para un Hard Reset o encendido manual
  if (digitalRead(button) == LOW) {
    delay(100);  // Debounce básico
    uint32_t startTime = millis();

    // Mientras el botón siga presionado...
    while (digitalRead(button) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        // Si lo mantienes presionado 3 segundos, se desvincula de la red
        Serial.println("Reiniciando Zigbee a valores de fábrica...");
        delay(1000);
        Zigbee.factoryReset();
      }
    }

    // Si fue un clic corto, alternamos la luz localmente (sin el switch)
    zbLight.setLight(!zbLight.getLightState());
  }

  delay(100);
}