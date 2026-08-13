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

// ==== CREACIÓN DE OBJETOS =====
StaticJsonDocument<1024> receiveJSON;
StaticJsonDocument<1024> sendJSON;
Adafruit_BNO055 bno(55, addrBNO055, &Wire);  // BNO055: orientación y movimiento
Adafruit_BMP280 bmp;                         // BMP280: temperatura y presión
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
  Wire.end();
  pinMode(SDA_PIN, INPUT);
  pinMode(SCL_PIN, INPUT);
  delay(100);
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
            bool status_bno055 = false;
            bool status_bmp238 = false;

            // ==== Inicialización de Sensores ====
            Wire.begin(SDA_PIN, SCL_PIN);
            // For higher and more stable speed, use the TLC4311. 400kHz
            Wire.setClock(400000);  // typical speed 100kHz
            delay(100);

            // Inicializar BNO055
            if (!bno.begin()) {
              sendJSON["status_bno"] = "FAIL";
              sendJSON["error_bno"] = "BNO055 I2C initialization failed.";
            } else {
              sendJSON["status_bno"] = "sensor initialized";
              status_bno055 = true;
            }
            delay(1500);  // Estabilizar BNO055

            // Inicializar BMP280 en dirección 0x76
            if (!bmp.begin(0x76)) {
              sendJSON["status_bmp"] = "FAIL";
              sendJSON["error_bmp"] = "BMP280 I2C initialization failed.";
            } else {
              sendJSON["status_bmp"] = "sensor initialized";
              status_bmp238 = true;
            }

            if (status_bno055 && status_bmp238) sendJSON["Result"] = "OK";
            delay(1000);  // Estabilización del BMP280

            // ==== Configuraciónd el Sensor ====
            bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                            Adafruit_BMP280::SAMPLING_X2,
                            Adafruit_BMP280::SAMPLING_X16,
                            Adafruit_BMP280::FILTER_X16,
                            Adafruit_BMP280::STANDBY_MS_500);

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:
          {
            for (int i = 0; i < noValues; i++) {

              // Eventos del BNO055
              sensors_event_t orientation, gyro, linearAccel, magnetometer, accel, gravity;
              bno.getEvent(&orientation, Adafruit_BNO055::VECTOR_EULER);
              bno.getEvent(&gyro, Adafruit_BNO055::VECTOR_GYROSCOPE);
              bno.getEvent(&linearAccel, Adafruit_BNO055::VECTOR_LINEARACCEL);
              bno.getEvent(&magnetometer, Adafruit_BNO055::VECTOR_MAGNETOMETER);
              bno.getEvent(&accel, Adafruit_BNO055::VECTOR_ACCELEROMETER);
              bno.getEvent(&gravity, Adafruit_BNO055::VECTOR_GRAVITY);

              // BMP280: temperatura y presión
              sensors_event_t temp_event, pressure_event;
              bmp_temp->getEvent(&temp_event);
              bmp_pressure->getEvent(&pressure_event);

              // Calibración BNO055
              int8_t temp_bno = bno.getTemp();
              uint8_t sys, gyroCal, accelCal, magCal;
              bno.getCalibration(&sys, &gyroCal, &accelCal, &magCal);

              // --- ESTRUCTURACIÓN DEL JSON ---
              StaticJsonDocument<512> doc;
              doc.clear();

              // BMP280
              doc["bmp280"]["temperature"] = temp_event.temperature;
              doc["bmp280"]["pressure"] = pressure_event.pressure;

              // BNO055 (Vectores)
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

              // Temperatura y Calibración
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




/*







*/