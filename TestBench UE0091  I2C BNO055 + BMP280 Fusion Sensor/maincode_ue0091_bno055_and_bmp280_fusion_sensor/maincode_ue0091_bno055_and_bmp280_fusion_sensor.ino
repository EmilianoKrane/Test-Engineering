/*
Firmware de Test UE0091 BNO055 + BMP280 Fusion Sensor
*/

// ==== LIBRERIAS ====
#include <Wire.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_BMP280.h>
#include <utility/imumaths.h>

// ==== DECLARACION DE GPIOS ====
#define RUN_BUTTON 4  // >> GPIO04 Botonera de Arranque
#define SDA_PIN 6     // >> GPIO06 SDA Bus I2C
#define SCL_PIN 7     // >> GPIO07 SCL Bus I2C

// ==== DECLARACIÓN DE VARIABLES GLOBALES ====
uint8_t addrBNO055 = 0x28;
int noValues = 20;
bool status_bno055 = false;  // Promovidas a globales para proteger las lecturas
bool status_bmp280 = false;

// ==== CREACIÓN DE OBJETOS =====
StaticJsonDocument<1024> receiveJSON;
StaticJsonDocument<1024> sendJSON;
Adafruit_BNO055 bno(55, addrBNO055, &Wire);
Adafruit_BMP280 bmp;
Adafruit_Sensor* bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor* bmp_pressure = bmp.getPressureSensor();

// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  StaticJsonDocument<255> doc;
  doc["debug"] = str;
  serializeJson(doc, Serial);
  Serial.println();
}

void releaseBuses() {
  Wire.end();                      // Libera el hardware I2C
  pinMode(SDA_PIN, INPUT_PULLUP);  // Asegura un estado alto seguro
  pinMode(SCL_PIN, INPUT_PULLUP);
  delay(50);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  sendJSON["System"] = "Ready";
  sendJSON["Module"] = "BNO055 + BMP280 DevLab";
  serializeJson(sendJSON, Serial);
  Serial.println();
}

void loop() {
  if (Serial.available()) {
    String inJSON = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inJSON);

    if (error) {
      sendJSON.clear();
      sendJSON["status"] = "FAIL";
      sendJSON["error"] = String("Invalid JSON: ") + error.c_str();
      serializeJson(sendJSON, Serial);
      Serial.println();
    } else {
      String Function = receiveJSON["Function"];
      int opc = 0;

      if (Function == "ping") opc = 1;             // {"Function":"ping"}
      else if (Function == "initSensor") opc = 2;  // {"Function":"initSensor"}
      else if (Function == "readSensor") opc = 3;  // {"Function":"readSensor"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();

            // 1. Limpiar el bus de lecturas previas atascadas
            releaseBuses();
            Wire.begin(SDA_PIN, SCL_PIN);
            Wire.setClock(100000);

// Timeout de hardware (Critico si usas arquitectura ESP32/RP2040)
#if defined(ESP32)
            Wire.setTimeOut(150);
#endif

            // 2. Inicializar BNO055
            status_bno055 = bno.begin();
            if (!status_bno055) {
              sendJSON["status_bno"] = "FAIL";
              sendJSON["error_bno"] = "BNO055 I2C initialization failed.";
            } else {
              sendJSON["status_bno"] = "sensor initialized";
              delay(50);  // Breve estabilización, no 500ms
            }

            // 3. Inicializar BMP280
            status_bmp280 = bmp.begin(0x76);
            if (!status_bmp280) {
              sendJSON["status_bmp"] = "FAIL";
              sendJSON["error_bmp"] = "BMP280 I2C initialization failed.";
            } else {
              sendJSON["status_bmp"] = "sensor initialized";
              // SOLUCIÓN AL CUELGUE: Solo configurar si la inicialización fue exitosa
              bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                              Adafruit_BMP280::SAMPLING_X2,
                              Adafruit_BMP280::SAMPLING_X16,
                              Adafruit_BMP280::FILTER_X16,
                              Adafruit_BMP280::STANDBY_MS_500);
              delay(50);
            }

            // Resultado final
            if (status_bno055 && status_bmp280) {
              sendJSON["Result"] = "OK";
            } else {
              sendJSON["Result"] = "FAIL";
            }

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:
          {
            // Protección: Evitar colgar el bus intentando leer sensores muertos
            if (!status_bno055 || !status_bmp280) {
              sendJSON.clear();
              sendJSON["status"] = "FAIL";
              sendJSON["error"] = "Sensors not properly initialized. Run initSensor first.";
              serializeJson(sendJSON, Serial);
              Serial.println();
              break;
            }

            for (int i = 0; i < noValues; i++) {
              sensors_event_t orientation, gyro, linearAccel, magnetometer, accel, gravity;
              bno.getEvent(&orientation, Adafruit_BNO055::VECTOR_EULER);
              bno.getEvent(&gyro, Adafruit_BNO055::VECTOR_GYROSCOPE);
              bno.getEvent(&linearAccel, Adafruit_BNO055::VECTOR_LINEARACCEL);
              bno.getEvent(&magnetometer, Adafruit_BNO055::VECTOR_MAGNETOMETER);
              bno.getEvent(&accel, Adafruit_BNO055::VECTOR_ACCELEROMETER);
              bno.getEvent(&gravity, Adafruit_BNO055::VECTOR_GRAVITY);

              sensors_event_t temp_event, pressure_event;
              bmp_temp->getEvent(&temp_event);
              bmp_pressure->getEvent(&pressure_event);

              int8_t temp_bno = bno.getTemp();
              uint8_t sys, gyroCal, accelCal, magCal;
              bno.getCalibration(&sys, &gyroCal, &accelCal, &magCal);

              StaticJsonDocument<512> doc;

              doc["bmp280"]["temperature"] = temp_event.temperature;
              doc["bmp280"]["pressure"] = pressure_event.pressure;

              doc["orientation"]["x"] = orientation.orientation.x;
              doc["orientation"]["y"] = orientation.orientation.y;
              doc["orientation"]["z"] = orientation.orientation.z;

              doc["gyroscope"]["x"] = gyro.gyro.x;
              doc["gyroscope"]["y"] = gyro.gyro.y;
              doc["gyroscope"]["z"] = gyro.gyro.z;

              doc["linear_accel"]["x"] = linearAccel.acceleration.x;
              doc["linear_accel"]["y"] = linearAccel.acceleration.y;
              doc["linear_accel"]["z"] = linearAccel.acceleration.z;

              doc["magnetometer"]["x"] = magnetometer.magnetic.x;
              doc["magnetometer"]["y"] = magnetometer.magnetic.y;
              doc["magnetometer"]["z"] = magnetometer.magnetic.z;

              doc["acceleration"]["x"] = accel.acceleration.x;
              doc["acceleration"]["y"] = accel.acceleration.y;
              doc["acceleration"]["z"] = accel.acceleration.z;

              doc["gravity"]["x"] = gravity.acceleration.x;
              doc["gravity"]["y"] = gravity.acceleration.y;
              doc["gravity"]["z"] = gravity.acceleration.z;

              doc["bno055_temp"] = temp_bno;
              doc["calibration"]["sys"] = sys;
              doc["calibration"]["gyro"] = gyroCal;
              doc["calibration"]["accel"] = accelCal;
              doc["calibration"]["mag"] = magCal;

              serializeJson(doc, Serial);
              Serial.println();
              delay(200);
            }
            break;
          }

        default:
          {
            sendJSON.clear();
            sendJSON["status"] = "FAIL";
            sendJSON["error"] = "Invalid function requested";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  }
}