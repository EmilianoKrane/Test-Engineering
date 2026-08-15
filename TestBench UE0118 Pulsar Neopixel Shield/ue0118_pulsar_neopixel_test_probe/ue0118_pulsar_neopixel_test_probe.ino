#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

#define PIN1 4  // Salida Neopixel Arnés
#define PIN2 5  // Salida Neopixel Punta de Prueba

#define BOTON_PIN 21  // Botón de Entrada

#define NUMPIXELS 258  // Popular NeoPixel ring size

Adafruit_NeoPixel pixels(NUMPIXELS, PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel neopixels(NUMPIXELS, PIN2, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 50  // Time (in milliseconds) to pause between pixels

// Dejamos una intensidad fija (puedes ajustarla de 0 a 255)
int intensity = 3;

// Variable para alternar la salida (0 = Arnés, 1 = Punta de Prueba)
int salidaActiva = 0;

// Variables para evitar el "rebote" (debounce) del botón
bool estadoBoton = HIGH;
bool ultimoEstadoBoton = HIGH;
unsigned long ultimoTiempoRebote = 0;
unsigned long retardoRebote = 50;  // 50 milisegundos

void setup() {
  Serial.begin(115200);

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  // Configuramos el botón con resistencia PULL-UP interna.
  pinMode(BOTON_PIN, INPUT_PULLUP);

  pixels.begin();
  neopixels.begin();

  // Brillo general al máximo, controlado por la variable "intensity" en los colores
  pixels.setBrightness(150);
  neopixels.setBrightness(150);

  // Aseguramos que ambas tiras inicien apagadas
  pixels.clear();
  pixels.show();
  neopixels.clear();
  neopixels.show();

  Serial.println("Inicializado...");
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

      // Si el botón está en LOW, es que fue presionado
      if (estadoBoton == LOW) {

        // Alternamos entre la salida 0 y la salida 1 (Toggle)
        salidaActiva = !salidaActiva;

        // Limpiamos ambas tiras de inmediato para que no se queden
        // LEDs encendidos de la animación anterior al hacer el cambio
        pixels.clear();
        pixels.show();
        neopixels.clear();
        neopixels.show();
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
  // Color Verde
  for (int i = 0; i < NUMPIXELS; i++) {
    if (salidaActiva == 0) {
      pixels.setPixelColor(i, pixels.Color(0, intensity, 0));
      pixels.show();
    } else {
      neopixels.setPixelColor(i, neopixels.Color(0, intensity, 0));
      neopixels.show();
    }
    smartDelay(DELAYVAL);
  }

  // Color Rojo
  for (int i = 0; i < NUMPIXELS; i++) {
    if (salidaActiva == 0) {
      pixels.setPixelColor(i, pixels.Color(intensity, 0, 0));
      pixels.show();
    } else {
      neopixels.setPixelColor(i, neopixels.Color(intensity, 0, 0));
      neopixels.show();
    }
    smartDelay(DELAYVAL);
  }

  // Color Azul
  for (int i = 0; i < NUMPIXELS; i++) {
    if (salidaActiva == 0) {
      pixels.setPixelColor(i, pixels.Color(0, 0, intensity));
      pixels.show();
    } else {
      neopixels.setPixelColor(i, neopixels.Color(0, 0, intensity));
      neopixels.show();
    }
    smartDelay(DELAYVAL);
  }
}