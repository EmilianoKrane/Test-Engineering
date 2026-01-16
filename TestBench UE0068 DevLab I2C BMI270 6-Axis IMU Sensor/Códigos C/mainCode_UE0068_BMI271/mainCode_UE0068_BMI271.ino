#include <SPI.h>
#include "SparkFun_BMI270_Arduino_Library.h"
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// ==== Declaración de pines
#define RX2 D4  // GPIO15 como RX
#define TX2 D5  // GPIO19 como TX

#define SDA_PIN 6   // MOSI
#define SCL_PIN 7   // SCl
#define SDO_PIN D1  // MISO
#define CS_PIN D0

// ==== Inicialización de objetos
HardwareSerial PagWeb(1);  // Objeto para UART2 en PULSAR como PagWeb
BMI270 imu;

// Variables de JSON
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

// Parámetros de SPI
uint8_t chipSelectPin = D0;
uint32_t clockFrequency = 100000;

void setup() {
  // Start serial
  Serial.begin(115200);

  // Iniciar UART2 en los pines seleccionados
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);

  Serial.println("MainCode JSON BMI270");
}

void loop() {

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {
      String Function = receiveJSON["Function"];  // Function es la variable de interés del JSON

      int opc = 0;

      if (Function == "scan") opc = 1;       // {"Function":"scan"}
      else if (Function == "SPI") opc = 2;   // {"Function":"SPI"}
      else if (Function == "ping") opc = 3;  // {"Function":"ping"}

      switch (opc) {
        case 1:
          {
            Serial.println("==== Escaneo y lectura en direcciones I2C ====");
            sendJSON.clear();  // Limpia cualquier dato previo

            String scan1 = scanI2C(68);
            Serial.println("Escaneo LOW: " + scan1);

            String scan2 = scanI2C(69);
            Serial.println("Escaneo HIGH: " + scan2);
            break;
          }

        case 2:
          {
            // Initialize the SPI library
            // void begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1);
            Serial.println("==== Escaneo y lectura SPI ====");
            sendJSON.clear();  // Limpia cualquier dato previo
            SPI.begin(SCL_PIN, D1, SDA_PIN, D0);

            for (int i = 0; i < 50; i++) {
              if (imu.beginSPI(chipSelectPin, clockFrequency) != BMI2_OK) {
                Serial.println("Error: BMI270 not connected, check wiring and CS pin!");
                sendJSON["SPI"] = "Fail";
              } else {
                // Inicialización del sensor en SPI
                sendJSON["SPI"] = "OK";
                for (int i = 0; i < 10; i++) {
                  readIMU();
                  delay(50);
                }
                return;
              }
              delay(50);
            }

            break;
          }

        case 3:
          {
            sendJSON.clear();  // Limpia cualquier dato previo
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
            PagWeb.println();
            break;
          }
      }
    }
  }
}



String scanI2C(int sw) {

  String addressI2C = "";

  pinMode(CS_PIN, INPUT);
  pinMode(SDO_PIN, OUTPUT);

  if (sw == 68) {
    digitalWrite(SDO_PIN, LOW);  // LOW = 0x68 || HIGH = 0x69
  } else if (sw == 69) {
    digitalWrite(SDO_PIN, HIGH);
  } else {
    digitalWrite(SDO_PIN, LOW);
  }

  Wire.begin(SDA_PIN, SCL_PIN);
  //Serial.println("Inicio de escáner I2C...");

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      //Serial.print("Dispositivo encontrado en 0x");

      if (addr == 0x68) {
        sendJSON["i2c0x68"] = "OK";
        sendJSON["addr"] = addr;
      }

      if (addr == 0x69) {
        sendJSON["i2c0x69"] = "OK";
        sendJSON["addr"] = addr;
      }

      if (addr < 16) Serial.print("0");
      //Serial.println(addr, HEX);

      addressI2C += "0x";
      if (addr < 16) addressI2C += "0";
      addressI2C += String(addr, HEX);
      addressI2C += " ";
    }
  }

  if (imu.beginI2C(0x68, Wire) == BMI2_OK) {
    Serial.println("BMI270 detectado en 0x68");
    delay(500);
    for (int i = 0; i < 10; i++) {
      readIMU();
      delay(50);
    }
  } else {
    sendJSON["i2c0x68"] = "Fail";
  }

  if (imu.beginI2C(0x69, Wire) == BMI2_OK) {
    Serial.println("BMI270 detectado en 0x69");
    for (int i = 0; i < 10; i++) {
      readIMU();
      delay(50);
    }
  } else {
    sendJSON["i2c0x69"] = "Fail";
  }

  Wire.end();

  return addressI2C;
}


void readIMU() {

  imu.getSensorData();

  Serial.print("ACC [g]  ");
  Serial.print("X: ");
  Serial.print(imu.data.accelX, 3);
  Serial.print("   ");

  Serial.print("Y: ");
  Serial.print(imu.data.accelY, 3);
  Serial.print("   ");

  Serial.print("Z: ");
  Serial.print(imu.data.accelZ, 3);
  Serial.print("   |   ");

  Serial.print("GYR [dps]  ");
  Serial.print("X: ");
  Serial.print(imu.data.gyroX, 3);
  Serial.print("   ");

  Serial.print("Y: ");
  Serial.print(imu.data.gyroY, 3);
  Serial.print("   ");

  Serial.print("Z: ");
  Serial.println(imu.data.gyroZ, 3);


  sendJSON["accelX"] = imu.data.accelX;
  sendJSON["accelY"] = imu.data.accelY;
  sendJSON["accelZ"] = imu.data.accelZ;

  sendJSON["gyroX"] = imu.data.gyroX;
  sendJSON["gyroY"] = imu.data.gyroY;
  sendJSON["gyroZ"] = imu.data.gyroZ;

  serializeJson(sendJSON, PagWeb);
  PagWeb.println();

  delay(100);
}
