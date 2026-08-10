#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_BMP280.h>
#include <utility/imumaths.h>

// BNO055: orientación y movimiento
Adafruit_BNO055 bno(55, 0x28, &Wire);

// BMP280: temperatura y presión
Adafruit_BMP280 bmp;
Adafruit_Sensor* bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor* bmp_pressure = bmp.getPressureSensor();

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Wire.begin(6, 7);

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      String addrHex = "";
      if (addr < 16) addrHex = "0";
      addrHex = addrHex + String(addr, HEX);
      Serial.println("I2C device found at 0x" + addrHex);
    }
  }

  // Inicializar BNO055
  if (!bno.begin()) {
    Serial.println("{\"error\":\"BNO055 not detected\"}");
    while (1)
      ;
  }
  delay(1000);  // Estabilizar BNO055

  // Inicializar BMP280 en dirección 0x76
  if (!bmp.begin(0x76)) {
    Serial.println("{\"error\":\"BMP280 not detected at 0x76\"}");
    while (1)
      ;
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
}

void loop() {
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

  // JSON OUTPUT
  Serial.print("{");

  // BMP280
  Serial.print("\"bmp280\":{\"temperature\":");
  Serial.print(temp_event.temperature, 2);
  Serial.print(",\"pressure\":");
  Serial.print(pressure_event.pressure, 2);
  Serial.print("},");

  // BNO055 - orientación
  Serial.print("\"orientation\":{\"x\":");
  Serial.print(orientation.orientation.x, 2);
  Serial.print(",\"y\":");
  Serial.print(orientation.orientation.y, 2);
  Serial.print(",\"z\":");
  Serial.print(orientation.orientation.z, 2);
  Serial.print("},");

  // BNO055 - giroscopio
  Serial.print("\"gyroscope\":{\"x\":");
  Serial.print(gyro.gyro.x, 2);
  Serial.print(",\"y\":");
  Serial.print(gyro.gyro.y, 2);
  Serial.print(",\"z\":");
  Serial.print(gyro.gyro.z, 2);
  Serial.print("},");

  // BNO055 - aceleración lineal
  Serial.print("\"linear_accel\":{\"x\":");
  Serial.print(linearAccel.acceleration.x, 2);
  Serial.print(",\"y\":");
  Serial.print(linearAccel.acceleration.y, 2);
  Serial.print(",\"z\":");
  Serial.print(linearAccel.acceleration.z, 2);
  Serial.print("},");

  // BNO055 - magnetómetro
  Serial.print("\"magnetometer\":{\"x\":");
  Serial.print(magnetometer.magnetic.x, 2);
  Serial.print(",\"y\":");
  Serial.print(magnetometer.magnetic.y, 2);
  Serial.print(",\"z\":");
  Serial.print(magnetometer.magnetic.z, 2);
  Serial.print("},");

  // BNO055 - aceleración total
  Serial.print("\"acceleration\":{\"x\":");
  Serial.print(accel.acceleration.x, 2);
  Serial.print(",\"y\":");
  Serial.print(accel.acceleration.y, 2);
  Serial.print(",\"z\":");
  Serial.print(accel.acceleration.z, 2);
  Serial.print("},");

  // BNO055 - gravedad
  Serial.print("\"gravity\":{\"x\":");
  Serial.print(gravity.acceleration.x, 2);
  Serial.print(",\"y\":");
  Serial.print(gravity.acceleration.y, 2);
  Serial.print(",\"z\":");
  Serial.print(gravity.acceleration.z, 2);
  Serial.print("},");

  // Temperatura interna y calibración
  Serial.print("\"bno055_temp\":");
  Serial.print(temp_bno);
  Serial.print(",");
  Serial.print("\"calibration\":{\"sys\":");
  Serial.print(sys);
  Serial.print(",\"gyro\":");
  Serial.print(gyroCal);
  Serial.print(",\"accel\":");
  Serial.print(accelCal);
  Serial.print(",\"mag\":");
  Serial.print(magCal);
  Serial.print("}");

  Serial.println("}");
  delay(500);
}