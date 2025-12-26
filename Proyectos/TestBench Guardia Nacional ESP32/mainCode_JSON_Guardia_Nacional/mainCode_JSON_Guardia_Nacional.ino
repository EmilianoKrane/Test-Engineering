// ==== CÓDIGO DE INTEGRACIÓN JUN R3 ====

#include <Adafruit_NeoPixel.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Wire.h>

// Declaración de Pines
#define PIN 8
#define LED_BUILTIN 13
#define LED_NEOP 15  // PIN DEDICADO AL NEOPIXEL WS2812

// Declaración de variables
//#define DELAYVAL 50

// Creación de Objetos
Adafruit_NeoPixel np = Adafruit_NeoPixel(1, LED_NEOP, NEO_GRB + NEO_KHZ800);  // Objeto NeoPixel

// JSON para recibir datos
String JSON;                        // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> datosJSON;  // Usa StaticJsonDocument para fijar tamaño

// JSON para enviar datos
String JSONCorriente;
StaticJsonDocument<200> enviarJSON;


void setup() {

  Serial.begin(115200);
  Serial.println("Serial inicializado...");

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {

  sequenceNeop();

  if (Serial.available()) {

    JSON = Serial.readStringUntil('\n');  // Leer hasta newline (JSON en crudo)

    // Deserializa el JSON y guarda la información en datosJSON
    DeserializationError error = deserializeJson(datosJSON, JSON);

    if (!error) {

      String Function = datosJSON["Function"];  // Function es la variable de interés del JSON

      int opc = 0;  // Variable de switcheo

      // Llaves JSON de Ejecución de pruebas

      // {"Function":"Run"}

      if (Function == "Run") opc = 1;

      switch (opc) {
        case 1:
          Serial.println("RUN");

          break;
      }
    }
  }

  else {
  }
}


// ==== Funciones Demo ====
void sequenceNeop() {
  np.setBrightness(20);
  np.setPixelColor(0, np.Color(255, 0, 0));  // Rojo
  np.show();
  delay(500);

  np.setPixelColor(0, np.Color(0, 255, 0));  // Verde
  np.show();
  delay(500);

  np.setPixelColor(0, np.Color(245, 73, 39));  // Rosa
  np.show();
  delay(500);

  np.setPixelColor(0, np.Color(0, 0, 255));  // Azul
  np.show();
  delay(500);

  np.setPixelColor(0, np.Color(0, 0, 0));  // Apagado
  np.show();
  delay(500);
}
