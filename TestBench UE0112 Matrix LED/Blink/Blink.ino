
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif


#define PIN1 6  // On Trinket or Gemma, suggest changing this to 1
#define PIN2 7  // On Trinket or Gemma, suggest changing this to 1
#define PIN3 8  // On Trinket or Gemma, suggest changing this to 1

#define NUMPIXELS 258  // Popular NeoPixel ring size

Adafruit_NeoPixel pixels(NUMPIXELS, PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel matrix(NUMPIXELS, PIN2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel neopixels(NUMPIXELS, PIN3, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 5  // Time (in milliseconds) to pause between pixels
int intensity = 3;

void setup() {

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  pixels.begin();
  matrix.begin();
  neopixels.begin();
}

void loop() {
  pixels.clear();
  matrix.clear();
  neopixels.clear();

  for (int i = 0; i < NUMPIXELS/2; i++) {
    pixels.setPixelColor(i, pixels.Color(0, intensity, 0));
    pixels.show();    // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);  // Pause before next pass through loop
  }

  for (int i = 0; i < NUMPIXELS/2; i++) {
    matrix.setPixelColor(i, matrix.Color(0, intensity, 0));
    matrix.show();  // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    neopixels.setPixelColor(i, neopixels.Color(0, intensity, 0));
    neopixels.show();  // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);
  }


  for (int i = 0; i < NUMPIXELS/2; i++) {
    pixels.setPixelColor(i, pixels.Color(intensity, 0, 0));
    pixels.show();    // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);  // Pause before next pass through loop
  }

  for (int i = 0; i < NUMPIXELS/2; i++) {
    matrix.setPixelColor(i, matrix.Color(intensity, 0, 0));
    matrix.show();  // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    neopixels.setPixelColor(i, neopixels.Color(intensity, 0, 0));
    neopixels.show();  // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);
  }


  for (int i = 0; i < NUMPIXELS/2; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, intensity));
    pixels.show();    // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);  // Pause before next pass through loop
  }

  for (int i = 0; i < NUMPIXELS/2; i++) {
    matrix.setPixelColor(i, matrix.Color(0, 0, intensity));
    matrix.show();  // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    neopixels.setPixelColor(i, neopixels.Color(0, 0, intensity));
    neopixels.show();  // Send the updated pixel colors to the hardware.
    delay(DELAYVAL);
  }
}
