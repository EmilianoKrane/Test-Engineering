/*
Firmware de Prueba UE0091 BNO055 + BMP280 Fusion Sensor
*/

// ==== LIBRERIAS ====
#include <Wire.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_BMP280.h>
#include <utility/imumaths.h>

#define SDA_PIN 6     // >> GPIO06 SDA Bus I2C
#define SCL_PIN 7     // >> GPIO07 SCL Bus I2C

StaticJsonDocument<1024> receiveJSON;
StaticJsonDocument<1024> sendJSON;

// BNO055: orientación y movimiento
Adafruit_BNO055 bno(55, 0x28, &Wire);

// BMP280: temperatura y presión
Adafruit_BMP280 bmp;
Adafruit_Sensor* bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor* bmp_pressure = bmp.getPressureSensor();

// ==== BANDERAS DE ESTADO GLOBAL ====
bool status_bno055 = false;
bool status_bmp280 = false;

// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  StaticJsonDocument<255> doc;
  doc["debug"] = str;
  serializeJson(doc, Serial);
  Serial.println();
}

void releaseBuses() {
  Wire.end();
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);
  delay(50);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  serialDebug("System Ready: BNO055 + BMP280 DevLab");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);  // High speed
  
  // Timeout para evitar que el bus se cuelgue si falta un sensor
  #if defined(ESP32) || defined(ESP8266)
  Wire.setTimeOut(150); 
  #endif
  
  delay(100);

  // Escaneo de Direcciones I2C con Logs JSON
  serialDebug("Scanning I2C bus...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      String addrHex = "";
      if (addr < 16) addrHex = "0";
      addrHex = addrHex + String(addr, HEX);
      serialDebug("I2C device found at 0x" + addrHex);
    }
  }

  // Inicializar BNO055
  serialDebug("Initializing BNO055...");
  status_bno055 = bno.begin();
  if (!status_bno055) {
    serialDebug("FAIL: BNO055 I2C initialization failed.");
  } else {
    serialDebug("OK: BNO055 initialized.");
  }
  delay(50); 

  // Inicializar BMP280 en dirección 0x76
  serialDebug("Initializing BMP280...");
  status_bmp280 = bmp.begin(0x76);
  if (!status_bmp280) {
    serialDebug("FAIL: BMP280 I2C initialization failed.");
  } else {
    serialDebug("OK: BMP280 initialized.");
    
    // Configurar muestreo solo si se inicializó correctamente
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
  }
  delay(50); 
  
  serialDebug("Setup complete. Starting loop...");
}


void loop() {
  StaticJsonDocument<768> doc; // Usamos un doc local para las lecturas
  doc.clear();

  // 1. Verificación inicial: ¿Sobrevivieron los sensores al setup?
  if (!status_bno055 || !status_bmp280) {
      doc["status"] = "FATAL_ERROR";
      doc["error"] = "Sensors not initialized. Halting reads.";
      serializeJson(doc, Serial);
      Serial.println();
      delay(2000);
      return; // Aborta el resto del loop para no saturar con basura
  }

  // 2. Intentar leer un evento (funciona como chequeo de salud)
  sensors_event_t orientation, gyro, linearAccel, magnetometer, accel, gravity;
  bool read_ok = bno.getEvent(&orientation, Adafruit_BNO055::VECTOR_EULER);
  
  // Si getEvent devuelve false, perdimos comunicación I2C
  if (!read_ok) {
      doc["status"] = "READ_ERROR";
      doc["error"] = "Failed to read from BNO055. Possible disconnection.";
      serializeJson(doc, Serial);
      Serial.println();
      delay(1000);
      return; // Aborta esta iteración
  }

  // Si llegamos aquí, la lectura es segura. Procedemos con el resto.
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

  // --- ESTRUCTURACIÓN DEL JSON ---
  doc["status"] = "OK"; // Indicador de lectura válida
  
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

  delay(500);
}