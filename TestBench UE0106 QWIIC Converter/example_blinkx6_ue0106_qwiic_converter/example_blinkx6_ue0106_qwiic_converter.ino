// GPIOS de UNIT Dual ONE RP2040


// ==== DEFINICIÓN DE PINES ====
/*
// GPIOS de UNIT Dual ONE RP2040

*/
#define LED1 1
#define LED2 0
#define LED3 5
#define LED4 4
#define LED5 9
#define LED6 11

/*
JUN R3

#define LED1 0
#define LED2 1
#define LED3 2
#define LED4 3
#define LED5 4
#define LED6 5
*/


// Sintaxis correcta de C++ para arreglos usando llaves {}
const int LEDS[] = { LED1, LED2, LED3, LED4, LED5, LED6 };
// Calculamos la cantidad de LEDs para que los bucles se adapten automáticamente
const int NUM_LEDS = sizeof(LEDS) / sizeof(LEDS[0]);

void setup() {
  // Iteramos sobre el arreglo para configurar todos los pines como salida
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LEDS[i], OUTPUT);
  }
}

void loop() {

  int delay_ms = 2000; 

  // 1. Encendemos los 6 LEDs al mismo tiempo
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LEDS[i], HIGH);
  }
  delay(delay_ms); 
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LEDS[i], LOW);
  }
  delay(delay_ms); 
}