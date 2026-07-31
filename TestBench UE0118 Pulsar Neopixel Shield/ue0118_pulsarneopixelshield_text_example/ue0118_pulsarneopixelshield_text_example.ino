#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define PIN1 22  // Matriz Izquierda
#define PIN2 21  // Matriz Derecha

Adafruit_NeoMatrix matriz1 = Adafruit_NeoMatrix(16, 8, PIN1,
  NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB + NEO_KHZ800);

Adafruit_NeoMatrix matriz2 = Adafruit_NeoMatrix(16, 8, PIN2,
  NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB + NEO_KHZ800);

// Escribe aquí la frase que quieras. ¡Puede ser tan larga como desees!
String mensaje = "Ing Pruebas UwU";

int x;             // Controla la posición actual del texto
int anchoTexto;    // Guardará cuántos píxeles mide el mensaje total

void setup() {
  matriz1.begin();
  matriz1.setTextWrap(false); // Importante para que el texto no salte de línea
  matriz1.setBrightness(40);

  matriz2.begin();
  matriz2.setTextWrap(false);
  matriz2.setBrightness(40);

  // Cada letra estándar en Adafruit_GFX mide 6 píxeles de ancho (5 de letra + 1 de espacio)
  anchoTexto = mensaje.length() * 6;
  
  // Iniciamos el texto totalmente a la derecha. 
  // Como tenemos dos matrices de 16 columnas, el ancho total imaginario es 32.
  x = 32; 
}

void loop() {
  matriz1.fillScreen(0);
  matriz2.fillScreen(0);

  // Configuramos el color (ej. Verde para ambas)
  matriz1.setTextColor(matriz1.Color(120, 0, 0));
  matriz2.setTextColor(matriz2.Color(120, 0, 0));

  // --- EL TRUCO MATEMÁTICO ---
  // matriz1 (Izquierda) dibuja el texto en la posición 'x'
  matriz1.setCursor(x, 0);
  matriz1.print(mensaje);

  // matriz2 (Derecha) dibuja el mismo texto, pero desplazado 16 píxeles a la izquierda
  matriz2.setCursor(x - 16, 0);
  matriz2.print(mensaje);

  matriz1.show();
  matriz2.show();

  // Movemos el texto un píxel hacia la izquierda
  x--;

  // Si el texto ya salió completamente por la izquierda de la matriz 1, 
  // lo reiniciamos a la derecha para que vuelva a entrar.
  if (x < -anchoTexto) {
    x = 32; 
  }

  // Ajusta este delay para cambiar la velocidad del scroll (menor número = más rápido)
  delay(70); 
}