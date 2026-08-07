#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

#define PIN1 6  // On Trinket or Gemma, suggest changing this to 1
#define PIN2 7  // On Trinket or Gemma, suggest changing this to 1
#define PIN3 8  // On Trinket or Gemma, suggest changing this to 1

#define BOTON_PIN 9  // Pin donde va conectado el botón

#define NUMPIXELS 258  // Popular NeoPixel ring size

Adafruit_NeoPixel pixels(NUMPIXELS, PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel matrix(NUMPIXELS, PIN2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel neopixels(NUMPIXELS, PIN3, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 1  // Time (in milliseconds) to pause between pixels

// Variables de intensidad
int intensity = 5;         // 50=Bajo, 150=Medio, 255=Alto
int estadoIntensidad = 0;  // 0: Bajo, 1: Medio, 2: Alto

// Variables para evitar el "rebote" (debounce) del botón
bool estadoBoton = HIGH;
bool ultimoEstadoBoton = HIGH;
unsigned long ultimoTiempoRebote = 0;
unsigned long retardoRebote = 50;  // 50 milisegundos

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  // Configuramos el botón con resistencia PULL-UP interna.
  // El botón debe conectarse entre el GPIO 9 y GND (Tierra).
  pinMode(BOTON_PIN, INPUT_PULLUP);

  pixels.begin();
  matrix.begin();
  neopixels.begin();

  // Dejamos el brillo general al máximo (255)
  // y controlaremos la luz real con tu variable "intensity"
  pixels.setBrightness(150);
  matrix.setBrightness(150);
  neopixels.setBrightness(150);
}

// Función que revisa si el botón fue presionado
void revisarBoton() {
  bool lectura = digitalRead(BOTON_PIN);

  if (lectura != ultimoEstadoBoton) {
    ultimoTiempoRebote = millis();
  }

  if ((millis() - ultimoTiempoRebote) > retardoRebote) {
    if (lectura != estadoBoton) {
      estadoBoton = lectura;

      // Si el botón está en LOW, es que fue presionado (por el PULL-UP)
      if (estadoBoton == LOW) {
        estadoIntensidad++;
        if (estadoIntensidad > 2) {
          estadoIntensidad = 0;
        }

        // Definimos los niveles de "Bajo, Medio y Alto"
        if (estadoIntensidad == 0) {
          intensity = 2;  // Bajo
        } else if (estadoIntensidad == 1) {
          intensity = 5;  // Medio
        } else {
          intensity = 10;  // Alto
        }
      }
    }
  }
  ultimoEstadoBoton = lectura;
}

// Reemplaza a delay(). Hace la pausa pero revisa el botón al mismo tiempo
void smartDelay(unsigned long ms) {
  unsigned long inicio = millis();
  while (millis() - inicio < ms) {
    revisarBoton();
  }
}

void loop() {
  pixels.clear();
  matrix.clear();
  neopixels.clear();

 

  for (int i = 0; i < NUMPIXELS / 2; i++) {
    pixels.setPixelColor(i, pixels.Color(0, intensity, 0));
    pixels.show();    
    smartDelay(DELAYVAL); // <-- Usamos nuestro delay inteligente
  }

  for (int i = 0; i < NUMPIXELS / 2; i++) {
    matrix.setPixelColor(i, matrix.Color(0, intensity, 0));
    matrix.show();  
    smartDelay(DELAYVAL);
  }


  for (int i = 0; i < NUMPIXELS; i++) {
    neopixels.setPixelColor(i, neopixels.Color(0, intensity, 0));
    neopixels.show();
    //smartDelay(DELAYVAL);
  }




  for (int i = 0; i < NUMPIXELS / 2; i++) {
    pixels.setPixelColor(i, pixels.Color(intensity, 0, 0));
    pixels.show();
    smartDelay(DELAYVAL);
  }

  for (int i = 0; i < NUMPIXELS / 2; i++) {
    matrix.setPixelColor(i, matrix.Color(intensity, 0, 0));
    matrix.show();
    smartDelay(DELAYVAL);
  }
  

  for (int i = 0; i < NUMPIXELS; i++) {
    neopixels.setPixelColor(i, neopixels.Color(intensity, 0, 0));
    neopixels.show();
    //smartDelay(DELAYVAL);
  }



  for (int i = 0; i < NUMPIXELS / 2; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, intensity));
    pixels.show();
    smartDelay(DELAYVAL);
  }

  for (int i = 0; i < NUMPIXELS / 2; i++) {
    matrix.setPixelColor(i, matrix.Color(0, 0, intensity));
    matrix.show();
    smartDelay(DELAYVAL);
  }
 

  for (int i = 0; i < NUMPIXELS; i++) {
    neopixels.setPixelColor(i, neopixels.Color(0, 0, intensity));
    neopixels.show();
    //smartDelay(DELAYVAL);
  }
}