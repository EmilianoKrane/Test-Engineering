#include <Arduino.h>
#include <SPI.h>
#include "bme68xLibrary.h"

// Pines personalizados para ESP32-C6
#define PIN_MOSI 7
#define PIN_MISO 2
#define PIN_SCK  6
#define PIN_CS   18

SPIClass mySPI(0);  // Bus SPI #0 para ESP32-C6
Bme68x bme;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Inicializar SPI con pines personalizados
  mySPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);

  // Iniciar BME688 en modo SPI
  bme.begin(PIN_CS, mySPI);

  if (bme.checkStatus() == BME68X_ERROR) {
    Serial.println("Error: no se pudo inicializar el sensor.");
    while (1);
  } else if (bme.checkStatus() == BME68X_WARNING) {
    Serial.println("Advertencia: " + bme.statusString());
  } else {
    Serial.println("Sensor BME688 SPI listo.");
  }

  bme.setTPH();
  bme.setHeaterProf(300, 100);

  Serial.println("Time(ms), Temp(°C), Pressure(Pa), Humidity(%), Gas(Ω), Status");
}

void loop() {
  bme68xData data;

  bme.setOpMode(BME68X_FORCED_MODE);
  delayMicroseconds(bme.getMeasDur());

  if (bme.fetchData()) {
    bme.getData(data);

    Serial.print(millis()); Serial.print(", ");
    Serial.print(data.temperature); Serial.print(", ");
    Serial.print(data.pressure); Serial.print(", ");
    Serial.print(data.humidity); Serial.print(", ");
    Serial.print(data.gas_resistance); Serial.print(", ");
    Serial.println(data.status, HEX);
  }

  delay(1000);
}


/*


#include <Arduino.h>
#include "bme68xLibrary.h"
#include <Wire.h>

#define SDA_PIN 6
#define SCL_PIN 7

Bme68x bme;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);  // 100 kHz

  // ✅ Solo llama a begin, sin usar if
  bme.begin(0x77, Wire);

  // Verifica estado del sensor
  if (bme.checkStatus() == BME68X_ERROR) {
    Serial.println("❌ Error: BME688 no detectado.");
    while (1);
  }

  Serial.println("✅ Sensor BME688 inicializado correctamente.");

  bme.setTPH();  // Temp, Pressure, Humidity
  bme.setHeaterProf(300, 100);  // Heater: 300°C, 100 ms

  Serial.println("Time(ms), Temp(°C), Pressure(Pa), Humidity(%), Gas(Ω), Status");
}

void loop() {
  bme68xData data;

  bme.setOpMode(BME68X_FORCED_MODE);
  delayMicroseconds(bme.getMeasDur());

  if (bme.fetchData()) {
    bme.getData(data);

    Serial.print(millis()); Serial.print(", ");
    Serial.print(data.temperature); Serial.print(", ");
    Serial.print(data.pressure); Serial.print(", ");
    Serial.print(data.humidity); Serial.print(", ");
    Serial.print(data.gas_resistance); Serial.print(", ");
    Serial.println(data.status, HEX);
  }

  delay(100);
}

*/




/*
// --- Ejemplo simplificado ---
#include <Arduino.h>
#include "bme68xLibrary.h"
#include <Wire.h>

#define SDA_PIN 6
#define SCL_PIN 7
Bme68x bme;

const int AVG_COUNT = 5;
float tempBuffer[AVG_COUNT];
int bufIdx = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  bme.begin(0x77, Wire);

  // Modo ambiente: Temp/Pressure/Humidity, sin heater
  bme.setTPH();          // desactiva gas/heater
  // NO llamar a setHeaterProf aquí si quieres temperatura ambiental precisa
  Serial.println("Time(ms), Temp(°C), Pressure(Pa), Humidity(%)");
}

float readAmbientTempAvg() {
  // Toma AVG_COUNT lecturas y devuelve promedio (simple)
  float sum = 0;
  for (int i = 0; i < AVG_COUNT; ++i) {
    bme.setOpMode(BME68X_FORCED_MODE);
    // esperar tiempo suficiente (ms). getMeasDur devuelve microsegundos
    delay(bme.getMeasDur() / 1000 + 50);
    bme68xData d;
    if (bme.fetchData()) {
      bme.getData(d);
      sum += d.temperature;
    } else {
      // si falla, no sumes - disminuir el divisor sería mejor, pero para simplicidad:
      --i; // reintenta
    }
    delay(20);
  }
  return sum / AVG_COUNT;
}

void loop() {
  float ambientT = readAmbientTempAvg();

  // Opcional: hacer medición de gas por separado (si quieres gas)
  // bme.setHeaterProf(300, 100); // activa heater, medir gas, pero no usar su T
  // ... medir gas ...

  Serial.print(millis()); Serial.print(", ");
  Serial.print(ambientT); Serial.print(", ");

  // Leer presión y humedad una sola vez rápida (ya las calculaste en el loop)
  // Para mostrar ejemplo, hacemos una lectura extra:
  bme.setOpMode(BME68X_FORCED_MODE);
  delay(bme.getMeasDur() / 1000 + 50);
  bme68xData data;
  if (bme.fetchData()) {
    bme.getData(data);
    Serial.print(data.pressure); Serial.print(", ");
    Serial.println(data.humidity);
  } else {
    Serial.println("err");
  }

  delay(1000);
}

*/