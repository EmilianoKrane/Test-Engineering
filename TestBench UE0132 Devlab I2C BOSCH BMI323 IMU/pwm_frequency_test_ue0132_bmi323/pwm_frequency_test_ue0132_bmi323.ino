/*
#include <Arduino.h>


#define SCK_PIN 6   // SPI SCK  / I2C SCL
#define MOSI_PIN 7  // SPI MOSI / I2C SDAs

int f = 0.1;
int delayms = 1/f;

void setup() {
  pinMode(SCK_PIN, OUTPUT);
  pinMode(MOSI_PIN, OUTPUT);
}

void loop() {
  digitalWrite(SCK_PIN, HIGH);
  digitalWrite(MOSI_PIN, HIGH);
  delayMicroseconds(f);
  digitalWrite(SCK_PIN, LOW);
  digitalWrite(MOSI_PIN, LOW);
  delayMicroseconds(f);
} 

*/
 
// 4M 5M 2M 3M 1M 500k 100k 50k 10k


#include <Arduino.h>
#include <cstdint>

// Definición del pin y parámetros de la señal
const uint8_t pulsePin = 6;
const uint32_t freq = 10000;  // 4 MHz
const uint8_t resolution = 2;   // Resolución de 2 bits (valores de 0 a 3)

void setup() {
  // Inicializa el pin con la frecuencia y resolución deseadas
  bool success = ledcAttach(pulsePin, freq, resolution);
  
  if (success) {
    // Para una resolución de 2 bits (2^2 = 4), la mitad es 2.
    // Esto configura el ciclo de trabajo (duty cycle) al 50%.
    ledcWrite(pulsePin, 2);
  } else {
    Serial.begin(115200);
    Serial.println("Error al configurar el PWM");
  }
}

void loop() {
  // El hardware mantiene el tren de pulsos de forma automática.
  // No necesitas poner nada aquí.
}