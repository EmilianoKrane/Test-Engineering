#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define NEO_PIN 1 
#define NUM_PIXELS 3

// 1. Agrupamos todos los pines en un arreglo constante
const uint8_t OUTPUT_PINS[] = {
  22, 26, 27, 28, 29, 8, 9, 10,       // LED_PIN, A0-A6
  19, 18, 17, 16, 15, 14, 13, 12,     // D0-D7
  21, 23, 20                          // D10-D12
};

// 2. Calculamos la cantidad total de pines automáticamente
const int NUM_PINS = sizeof(OUTPUT_PINS) / sizeof(OUTPUT_PINS[0]);

Adafruit_NeoPixel np(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

void sweepGPIOS() {
  int delay_ms = 1000;
  
  // Encender todos los pines
  for (int i = 0; i < NUM_PINS; i++) {
    digitalWrite(OUTPUT_PINS[i], HIGH);
  }
  delay(delay_ms);
  
  // Apagar todos los pines
  for (int i = 0; i < NUM_PINS; i++) {
    digitalWrite(OUTPUT_PINS[i], LOW);
  }
  delay(delay_ms);
}

void setup() {
  // Configurar todos los pines como salida usando el bucle
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(OUTPUT_PINS[i], OUTPUT);
  }

  np.begin();                               // Inicialización del objeto NeoPixel
  np.clear();                               // Limpiar estado inicial (apagar todos los LEDs)
  np.setPixelColor(0, np.Color(20, 0, 0));  // Rojo
  np.setPixelColor(1, np.Color(0, 20, 0));  // Verde (Nota: en tus comentarios decía Rojo)
  np.setPixelColor(2, np.Color(0, 0, 20));  // Azul (Nota: en tus comentarios decía Rojo)
  np.show();                                // Mostrar cambios
}

void loop() {
  sweepGPIOS();
}