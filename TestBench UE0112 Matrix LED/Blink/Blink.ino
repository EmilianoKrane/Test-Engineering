
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif


#define PIN1 6  // On Trinket or Gemma, suggest changing this to 1
#define PIN2 7  // On Trinket or Gemma, suggest changing this to 1

#define NUMPIXELS 2  // Popular NeoPixel ring size

Adafruit_NeoPixel pixels(NUMPIXELS, PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel matrix(NUMPIXELS, PIN2, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 500  // Time (in milliseconds) to pause between pixels

void setup() {

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  pixels.begin();
  matrix.begin();
}

void loop() {
  pixels.clear();
  matrix.clear();

  // Apagado de segunda matriz
  for (int i = 0; i < NUMPIXELS; i++) {
    matrix.setPixelColor(i, matrix.Color(0, 0, 0));
    matrix.show();  // Send the updated pixel colors to the hardware.
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 50, 0));
    pixels.show();    // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);  // Pause before next pass through loop
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    matrix.setPixelColor(i, matrix.Color(0, 50, 0));
    matrix.show();  // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);
  }


  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(50, 0, 0));
    pixels.show();    // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);  // Pause before next pass through loop
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    matrix.setPixelColor(i, matrix.Color(50, 0, 0));
    matrix.show();  // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);
  }


  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 50));
    pixels.show();    // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);  // Pause before next pass through loop
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    matrix.setPixelColor(i, matrix.Color(0, 0, 50));
    matrix.show();  // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);
  }
}
