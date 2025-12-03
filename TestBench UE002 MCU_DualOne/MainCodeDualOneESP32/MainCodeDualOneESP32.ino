/* 
===== CÓDIGO UNIT DUAL ONE JSON Integración ESP32 ====
*/

#include <HardwareSerial.h>

#define RX2 16
#define TX2 17

#define LED_R 25
#define LED_G 26
#define LED_B 27


HardwareSerial Bridge(1);

void setup() {
  Serial.begin(115200);
  Serial.println("UART0 listo para comunicación...");

  Bridge.begin(115200, SERIAL_8N1, RX2, TX2);

  pinMode(LED_R, OUTPUT);  // Activos a BAJA
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
}

void loop() {


  if (Serial.available()) {       // If anything comes in Serial (USB),
    Bridge.write(Serial.read());  // read it and send it out Serial1 (pins 0 & 1)
  }

  if (Bridge.available()) {       // If anything comes in Serial1 (pins 0 & 1)
    Serial.write(Bridge.read());  // read it and send it out Serial (USB)
  }

  if (!Serial.available()) {  // If anything comes in Serial (USB),
    demo();
  }
}

/*
{"Function":"meas"}
*/


void demo() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);

  delay(500);
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, HIGH);

  delay(500);
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, LOW);

  delay(500);
}