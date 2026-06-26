/* 
Firmware MultiHub Shield - Parte Lógica 
*/


// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// ==== DECLARACIÓN DE GPIOS ====
#define NEOP_PIN 8     // >> GPIO08 NEOPIXEL
#define BUTTON_PIN 20  // >> GPIO15 Lectura de botón en shield

// ==== DECLARACIÓN DE VARIABLES GLOBALES ====
#define NUMPIXELS 26
#define DELAYVAL 20  // Time (in milliseconds) to pause between pixels

// ==== CREACIÓN DE OBJETOS ====
Adafruit_NeoPixel pixels(NUMPIXELS, NEOP_PIN, NEO_GRB + NEO_KHZ800);


void demo() {
  // ==== LECTURA DE BOTONES ====
  if (digitalRead(BUTTON_PIN) == LOW) {
    pixels.clear();
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 10, 0));
      pixels.show();
      delay(DELAYVAL);
    }
  } else {
    // ==== SECUENCIA DE NEOPIXEL ====
    pixels.clear();
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 10));
      pixels.show();
      delay(DELAYVAL);
    }
  }
}

void setup() {
  // ==== Inicializaciones ====
  Serial.begin(115200);
  pixels.begin();
  delay(100);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (husb238.begin(HUSB238_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("HUSB238 initialized successfully.");
  } else {
    Serial.println("Couldn't find HUSB238, check your wiring?");
  }

  // ==== Declaración de GPIOS ====
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  demo();
}
