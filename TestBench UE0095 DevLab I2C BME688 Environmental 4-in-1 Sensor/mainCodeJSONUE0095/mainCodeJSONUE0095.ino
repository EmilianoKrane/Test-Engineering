/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL UE0095 I2C SPI Sensor 4 en 1 BME688 JSON ====
*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <SPI.h>
#include "bme68xLibrary.h"


// ==== Declaración de pines
#define RX2 D4  // GPIO15 como RX
#define TX2 D5  // GPIO19 como TX

#define PIN_MOSI 7  // Comunicación SPI con el sensor
#define PIN_MISO 2
#define PIN_SCK 6
#define PIN_CS 3

// ==== Inicialización de objetos
HardwareSerial PagWeb(1);  // Objeto para UART2 en PULSAR como PagWeb
SPIClass mySPI(0);         // Bus SPI #0 para ESP32-C6
Bme68x bme;

// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;


void setup() {

  // Iniciar UART0 (USB) para depuración
  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");

  // Iniciar UART2 en los pines seleccionados
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("UART2 iniciado en RX=9, TX=8");

  // Inicializar SPI con pines personalizados
  mySPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);

  // Iniciar BME688 en modo SPI
  bme.begin(PIN_CS, mySPI);

  if (bme.checkStatus() == BME68X_ERROR) {
    Serial.println("Error: no se pudo inicializar el sensor.");
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

  if (PagWeb.available()) {
    JSON_entrada = PagWeb.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {

      String Function = receiveJSON["Function"];  // Function es la variable de interés del JSON

      int opc = 0;
      if (Function == "Lectura") opc = 1;

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            String status = "Fail";

            // Verificar si el sensor está conectado
            if (!sensorBMEDisponible()) {
              Serial.println("BME688 no detectado.");

              sendJSON["temp"] = "N/A";
              sendJSON["press"] = "N/A";
              sendJSON["hum"] = "N/A";
              sendJSON["gas"] = "N/A";
              sendJSON["status"] = "Fail";

              serializeJson(sendJSON, PagWeb);
              PagWeb.println();
              break;
            }

            // Reconfigurar antes de medir
            bme.setTPH();
            bme.setHeaterProf(300, 100);

            float avgTemp = 0;
            float avgPress = 0;
            float avgHum = 0;
            float avgGas = 0;

            bme68xData data;

            // Tomar 10 mediciones
            for (int i = 0; i < 10; i++) {

              bme.setOpMode(BME68X_FORCED_MODE);
              delayMicroseconds(bme.getMeasDur());

              if (bme.fetchData()) {
                bme.getData(data);
                avgTemp += data.temperature;
                avgPress += data.pressure;
                avgHum += data.humidity;
                avgGas += data.gas_resistance;
              }

              delay(150);
            }

            avgTemp /= 10;
            avgPress /= 10;
            avgHum /= 10;
            avgGas /= 10;

            if (avgTemp > 15 && avgTemp < 40 && avgPress > 50000 && avgPress < 90000 && avgHum >= 0 && avgHum <= 100) {
              status = "OK";
            }

            String dataTemp = String(avgTemp) + " °C";
            String dataPress = String(avgPress) + " Pa";
            String dataHum = String(avgHum) + " %"; 

            sendJSON["temp"] = dataTemp;
            sendJSON["press"] = dataPress;
            sendJSON["hum"] = dataHum;
            sendJSON["gas"] = avgGas;
            sendJSON["status"] = status;

            serializeJson(sendJSON, PagWeb);
            PagWeb.println();

            break;
          }
      }
    }
  }
}


bool sensorBMEDisponible() {
  bme.begin(PIN_CS, mySPI);
  return (bme.checkStatus() == BME68X_OK);
}



/*
    bme68xData data;

  bme.setOpMode(BME68X_FORCED_MODE);
  delayMicroseconds(bme.getMeasDur());

  if (bme.fetchData()) {
    bme.getData(data);

    Serial.print(millis());
    Serial.print(", ");
    Serial.print(data.temperature);
    Serial.print(", ");
    Serial.print(data.pressure);
    Serial.print(", ");
    Serial.print(data.humidity);
    Serial.print(", ");
    Serial.print(data.gas_resistance);
    Serial.print(", ");
    Serial.println(data.status, HEX);
  }
  */



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