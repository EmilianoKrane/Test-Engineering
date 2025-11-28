#include <Wire.h>
#include <Adafruit_DRV2605.h>

//////////////////// Pines / I2C ////////////////////
#define SDA_PIN 6
#define SCL_PIN 7

Adafruit_DRV2605 drv;
bool drvConnected = false;

//////////////////// Funciones de secuencia ////////////////////

void hapticMode1() {
  drv.setWaveform(0, 118);  // vibración fuerte
  drv.setWaveform(1, 0);    // fin
  drv.go();
  delay(100);
}

//////////////////// Setup ////////////////////
void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Iniciando búsqueda de DRV2605L...");
}

//////////////////// Loop ////////////////////
void loop() {
  // Verificar si el dispositivo responde en 0x5A
  Wire.beginTransmission(0x5A);
  bool found = (Wire.endTransmission() == 0);

  if (found) {
    if (!drvConnected) {
      Serial.println("DRV2605L detectado. Reconfigurando bus...");

      // Reiniciar bus I2C por seguridad
      Wire.end();
      delay(50);
      Wire.begin(SDA_PIN, SCL_PIN);
      delay(50);

      // Reiniciar e inicializar chip
      if (drv.begin(&Wire)) {
        drv.selectLibrary(1);
        drv.useERM();
        drv.setMode(DRV2605_MODE_INTTRIG);
        drvConnected = true;
        Serial.println("DRV2605L inicializado correctamente.");
      } else {
        Serial.println("Error al inicializar DRV2605L.");
        drvConnected = false;
      }
    }

    if (drvConnected) {
      Serial.println("Modo 1...");
      hapticMode1();
    }

  } else {
    if (drvConnected) {
      Serial.println("DRV2605L desconectado.");
      drvConnected = false;
    }
  }

  delay(700);
}



/*





#include <Wire.h>
#include <Adafruit_DRV2605.h>

//////////////////// Pines / I2C ////////////////////
#define SDA_PIN 22
#define SCL_PIN 23

Adafruit_DRV2605 drv;

//////////////////// Funciones de secuencia ////////////////////

// Modo 1: Vibración fuerte + pausa + pulso
void hapticMode1() {
  drv.setWaveform(0, 118);   // vibración fuerte
  drv.setWaveform(1, 0);    // fin
  drv.go();
  delay(300);

  drv.setWaveform(0, 47);   // pulso corto
  drv.setWaveform(1, 0);
  drv.go();
}

// Modo 2: Doble pulso rápido (notificación corta)
void hapticMode2() {
  drv.setWaveform(0, 47);   // pulso corto
  drv.setWaveform(1, 47);   // otro pulso corto
  drv.setWaveform(2, 0);
  drv.go();
}

// Modo 3: Patrón tipo alarma (fuerte → zumbido largo → fuerte)
void hapticMode3() {
  drv.setWaveform(0, 85);   // fuerte
  drv.setWaveform(1, 14);   // zumbido largo
  drv.setWaveform(2, 85);   // fuerte otra vez
  drv.setWaveform(3, 0);
  drv.go();
}

//////////////////// Setup ////////////////////
void setup() {
  Serial.begin(115200);
  delay(200);

#if defined(ARDUINO_ARCH_ESP32)
  Wire.begin(SDA_PIN, SCL_PIN);  // I2C en ESP32
#else
  Wire.begin();
#endif

  if (!drv.begin(&Wire)) {
    Serial.println("No se encontró DRV2605L en 0x5A");
    while (1) delay(10);
  }

  drv.selectLibrary(1);   // Librería de efectos
  drv.useERM();           // ERM por defecto
  drv.setMode(DRV2605_MODE_INTTRIG);

  Serial.println("Haptic listo (usa modos 1, 2, 3)...");
}

//////////////////// Loop ////////////////////
void loop() {
  Serial.println("Modo 1...");
  hapticMode1();

}
*/