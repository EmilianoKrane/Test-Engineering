/*
Este firmware se carga en la JUNR3 para dar salidas digitales de 0-5V
*/

#include <HardwareSerial.h>
#include "Arduino.h"

#define A1_PIN PD2
#define A2_PIN PD3
#define A3_PIN PD4
#define A4_PIN PD5

const int delay_ms = 2000;

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(A1_PIN, OUTPUT);
  pinMode(A2_PIN, OUTPUT);
  pinMode(A3_PIN, OUTPUT);
  pinMode(A4_PIN, OUTPUT);

  Serial.println("Serial inicializado en JUNR3");
}

void loop() {
  Serial.println("Estado ALTO");
  digitalWrite(A1_PIN, HIGH);
  digitalWrite(A2_PIN, HIGH);
  digitalWrite(A3_PIN, HIGH);
  digitalWrite(A4_PIN, HIGH);
  delay(delay_ms);

  Serial.println("Estado BAJO");
  digitalWrite(A1_PIN, LOW);
  digitalWrite(A2_PIN, LOW);
  digitalWrite(A3_PIN, LOW);
  digitalWrite(A4_PIN, LOW);
  delay(delay_ms);
}
