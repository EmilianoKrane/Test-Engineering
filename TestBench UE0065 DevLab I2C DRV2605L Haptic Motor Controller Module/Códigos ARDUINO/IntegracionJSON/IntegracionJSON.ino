// === CÓDIGO DE INTEGRACIÓN CON JSON para UE0065 DevLab: I2C DRV2605L Haptic Motor Controller Module

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <Adafruit_DRV2605.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// Pines para UART2
#define RX2 15  // GPIO15 como RX
#define TX2 19  // GPIO19 como TX

// Comunicación UART Creación Objeto
HardwareSerial PagWeb(1);  // Crear objeto para UART2 en PULSAR como PagWeb

// Pines I2C para comunicación con módulo
#define SDA_PIN 6
#define SCL_PIN 7

Adafruit_DRV2605 drv;

// JSON para recibir datos
String receiveJSON;                 // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> datosJSON;  // Usa StaticJsonDocument para fijar tamaño

// JSON para enviar datos
String sendJSON;
StaticJsonDocument<200> enviarJSON;


// ==== SETUP
void setup() {

  // Inicializar puertos serie
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("No está habilitado el UART1");

  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("UART2 iniciado!");

#if defined(ARDUINO_ARCH_ESP32)
  Wire.begin(SDA_PIN, SCL_PIN);  // I2C en ESP32
#else
  Wire.begin();
#endif

  if (!drv.begin(&Wire)) {
    Serial.println("No se encontró DRV2605L en 0x5A");
    while (1) delay(10);
  }

  drv.selectLibrary(1);  // Librería de efectos
  drv.useERM();          // ERM por defecto
  drv.setMode(DRV2605_MODE_INTTRIG);

  Serial.println("Haptic listo (usa modos 1, 2, 3)...");
}

//////////////////// Loop ////////////////////
void loop() {
  if (PagWeb.available()) {

    receiveJSON = PagWeb.readStringUntil('\n');                            // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(datosJSON, receiveJSON);  // receiveJSON -> datosJSON

    // No hay error de comunicación
    if (!error) {
      String Function = datosJSON["Function"];  // Function obtiene lo recibido en la clave

      int i = 0;  // Variable de switcheo 

      if (Function == "Mode1") i = 1;
      else if (Function == "Mode2") i = 2;
      else if (Function == "Mode3") i = 3;

      switch (i) {
        case 1:
          Serial.println("Modo 1");
          hapticMode1();
          break;

        case 2:
          Serial.println("Modo 2");
          hapticMode2();
          break;

        case 3:
          Serial.println("Modo 3");
          hapticMode3();
          break;
      }




    } else {
      Serial.print("Error de JSON: ");
      Serial.println(error.c_str());
    }



  } else {
    // Si no hay comunicación por PagWeb
  }
}


// ===== FUNCIONES DE OPERACIÓN =====
// Modo 1: Vibración fuerte + pausa + pulso
void hapticMode1() {
  drv.setWaveform(0, 85);  // vibración fuerte
  drv.setWaveform(1, 0);   // fin
  drv.go();
  delay(300);

  drv.setWaveform(0, 47);  // pulso corto
  drv.setWaveform(1, 0);
  drv.go();
}

// Modo 2: Doble pulso rápido (notificación corta)
void hapticMode2() {
  drv.setWaveform(0, 47);  // pulso corto
  drv.setWaveform(1, 47);  // otro pulso corto
  drv.setWaveform(2, 0);
  drv.go();
}

// Modo 3: Patrón tipo alarma (fuerte → zumbido largo → fuerte)
void hapticMode3() {
  drv.setWaveform(0, 85);  // fuerte
  drv.setWaveform(1, 14);  // zumbido largo
  drv.setWaveform(2, 85);  // fuerte otra vez
  drv.setWaveform(3, 0);
  drv.go();
}
