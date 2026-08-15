#include "DevLab_Test_PulsarC6.h"

// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  sendJSON.clear();
  sendJSON["debug"] = str;
  serializeJson(sendJSON, Serial);
  Serial.println();
}

bool testGpios(uint8_t gpioA, uint8_t gpioB) {
  bool resultAB = testSequence(gpioA, gpioB);
  if (!resultAB) return false;

  delay(10);  // Pausa entre pruebas

  bool resultBA = testSequence(gpioB, gpioA);
  if (!resultBA) return false;

  return true;  // Ambas pruebas correctas
}

bool testSequence(uint8_t gpioOut, uint8_t gpioIn) {
  pinMode(gpioOut, OUTPUT);
  pinMode(gpioIn, INPUT);  // Configuración con resistencia

  uint8_t testPattern[] = { 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1 };

  for (int i = 0; i < sizeof(testPattern); i++) {
    digitalWrite(gpioOut, testPattern[i]);  // Envía bit
    delay(10);                              // Espera de estabilización

    int readValue = digitalRead(gpioIn);  // Lee bit

    if (readValue != testPattern[i]) {
      return false;
    }
  }
  return true;  // Todos los bits coincidieron correctamente
}

bool i2cCheckDevice(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);
}

void demo() {
  int delay_ms = 100;
  int neop = 1;

  digitalWrite(SDA_PIN, HIGH);
  delay(delay_ms);

  // ---- Neopixel en Rojo ----
  for (int i = 0; i < neop; i++) {
    pixels.setPixelColor(i, pixels.Color(intensity, 0, 0));
    pixels.show();
    delay(delay_ms);
  }

  digitalWrite(SDA_PIN, LOW);
  delay(delay_ms);

  // ---- Neopixel en Verde ----
  for (int i = 0; i < neop; i++) {
    pixels.setPixelColor(i, pixels.Color(0, intensity, 0));
    pixels.show();
    delay(delay_ms);
  }

  digitalWrite(SDA_PIN, HIGH);
  delay(delay_ms);

  // ---- Neopixel en Azul----
  for (int i = 0; i < neop; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, intensity));
    pixels.show();
    delay(delay_ms);
  }
  pixels.clear();

  digitalWrite(SDA_PIN, LOW);
  delay(delay_ms);
}

