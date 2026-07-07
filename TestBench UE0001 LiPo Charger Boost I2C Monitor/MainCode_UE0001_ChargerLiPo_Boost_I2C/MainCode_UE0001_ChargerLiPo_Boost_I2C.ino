/*

*/

// ====   BIBLIOTECAS ====
#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_INA219.h>

// ==== DECLARACIÓN DE GPIOS ====
#define RUN_BUTTON 4  // >> GPIO04 Arranque por Botonera en TestBench
#define SDA_PIN 6     // >> GPIO06 Bus de datos I2C SDA
#define SCL_PIN 7     // >> GPIO07 Señal de reloj I2D SCL
#define RELAYA 8      // >> GPIO08 Accionamiento de Relé A Fuente de Alimentación [+]
#define RELAYB 9      // >> GPIO09 Accionamiento de Relé B Fuente de Alimentación [-]


// ==== DECLARACIÓN DE OBJETOS ====
String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;                ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas

Adafruit_INA219 ina219_in(0x40);   // Sensor de corriente INA219 en entrada del testbench
Adafruit_INA219 ina219_out(0x41);  // Sensor de corriente INA219 en salida del testbench


// ==== DECLARACIÓN DE REGISTRO Y VARIABLES GLOBALES ====
#define MAX17048_ADDR 0x36
#define REG_VCELL 0x02
#define REG_SOC 0x04
#define REG_MODE 0x06              // Registro para comandos especiales
const float shuntOffset_mV = 0.0;  // Offset en vacío para lectura inicial
const float R_SHUNT = 0.05;        // Resistencia Shunt = 50 mΩ
float corrienteSensor = 0;         // Variable de lectura de corriente con el sensor
float voltajeSensor = 0;           // Variable de lectura de voltaje con el sensor


// ==== UTILIDADES ====
void serialDebug(String str) {
  StaticJsonDocument<200> debugDoc;
  debugDoc["debug"] = str;
  serializeJson(debugDoc, Serial);
  Serial.println();
}

// ---- Función segura para leer registro de MAX17048 ----
uint16_t readRegister16(uint8_t reg) {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(reg);

  // Guardamos el resultado con Repeated Start
  uint8_t error = Wire.endTransmission(false);

  if (error != 0) {
    // FIX VITAL: Si falló, forzamos un STOP manual (true) para liberar la
    // máquina de estados del ESP32 antes de escapar.
    Wire.endTransmission(true);
    return 0;
  }

  uint8_t bytesReceived = Wire.requestFrom((uint16_t)MAX17048_ADDR, (uint8_t)2);

  if (bytesReceived == 2) {
    uint16_t msb = Wire.read();
    uint16_t lsb = Wire.read();
    return (msb << 8) | lsb;
  }

  return 0;
}

float readVoltage() {
  uint16_t rawVcell = readRegister16(REG_VCELL);
  return rawVcell * 0.000078125;
}

float readSOC() {
  uint16_t rawSoc = readRegister16(REG_SOC);
  return rawSoc / 256.0;
}

// ---- Función segura de QuickStart ----
bool sendQuickStart() {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(REG_MODE);
  Wire.write(0x40);  // MSB del comando 0x4000
  Wire.write(0x00);  // LSB del comando 0x4000

  // endTransmission devuelve 0 si el esclavo respondió correctamente (ACK)
  uint8_t error = Wire.endTransmission();

  if (error == 0) {
    return true;  // Comunicación exitosa
  } else {
    return false;  // Fallo en la comunicación (target desconectado o error de bus)
  }
}

/**
 * @brief Obtiene la corriente medida por el INA219 de entrada.
 * @return Corriente en amperios.
 */
float current_in() {
  float shunt_mV = ina219_in.getShuntVoltage_mV();
  float bus_V = ina219_in.getBusVoltage_V();

  shunt_mV -= shuntOffset_mV;
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT;  // Corriente de interés
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;
  return current_A;
}

/**
 * @brief Obtiene la corriente medida por el INA219 de salida.
 * @return Corriente en amperios.
 */
float current_out() {
  float shunt_mV = ina219_out.getShuntVoltage_mV();
  float bus_V = ina219_out.getBusVoltage_V();

  shunt_mV -= shuntOffset_mV;
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT;  // Corriente de interés
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;
  return current_A;
}


/**
 * @brief Rutina de recuperación de hardware para destrabar el bus I2C.
 * Obliga a cualquier esclavo colgado a soltar la línea SDA.
 */
void recoverI2CBus() {
  serialDebug("Iniciando recuperacion del bus I2C...");

  // 1. Desactivamos el hardware I2C del ESP32 para tomar control manual
  Wire.end();

  // 2. Configuramos los pines como I/O estándar
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, OUTPUT);
  digitalWrite(SCL_PIN, HIGH);
  delay(1);

  // 3. Verificamos si alguien está forzando SDA a LOW
  if (digitalRead(SDA_PIN) == LOW) {
    serialDebug("SDA detectado en LOW. Enviando pulsos de reloj para destrabar...");

    // Enviamos hasta 9 pulsos de reloj (es lo máximo que dura un byte + ACK)
    for (int i = 0; i < 9; i++) {
      digitalWrite(SCL_PIN, LOW);
      delayMicroseconds(10);
      digitalWrite(SCL_PIN, HIGH);
      delayMicroseconds(10);

      // Si el esclavo ya soltó la línea (SDA = HIGH), ya no necesitamos más pulsos
      if (digitalRead(SDA_PIN) == HIGH) {
        break;
      }
    }
  }

  // 4. Generamos una condición de STOP manual (SCL alto, luego SDA de bajo a alto)
  pinMode(SDA_PIN, OUTPUT);
  digitalWrite(SDA_PIN, LOW);
  delayMicroseconds(10);
  digitalWrite(SCL_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(SDA_PIN, HIGH);
  delay(5);

  // 5. Reiniciamos el hardware I2C y reinstalamos el Timeout
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setTimeOut(50);
  serialDebug("Bus I2C reiniciado.");
}

void setup() {
  Serial.begin(115200);
  delay(100);
  serialDebug("Serial initialized...");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setTimeOut(50);

  if (!ina219_in.begin(&Wire)) {
    serialDebug("Current sensor INA219_out 0x40 no initilized...");
  }

  if (!ina219_out.begin(&Wire)) {
    serialDebug("Current sensor INA219_out 0x41 no initilized...");
  }

  // ---- Configuración de GPIOS ----
  pinMode(RUN_BUTTON, INPUT_PULLDOWN);
  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);

  digitalWrite(RELAYA, LOW);  // >> Relevador de Fuente ON (Activo BAJAS)
  digitalWrite(RELAYB, LOW);  // >> Relevador de Fuente ON (Activo BAJAS)
  delay(500);
}

void loop() {

  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();
    delay(100);
    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON["Run"] = "OK";
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
  }

  if (Serial.available()) {
    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "ping") opc = 1;                // {"Function": "ping"}
      else if (Function == "monitorMax") opc = 2;     // {"Function": "monitorMax"}
      else if (Function == "currentSensor") opc = 3;  // {"Function": "currentSensor"}

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

            // 1. Verificamos pasivamente el estado de conexión del target
            Wire.beginTransmission(MAX17048_ADDR);
            uint8_t busStatus = Wire.endTransmission();

            if (busStatus == 0) {
              // 2. Target presente (ACK recibido). Procedemos con la lógica.
              bool commStatus = sendQuickStart();
              delay(100);

              if (commStatus) {
                sendQuickStart();
                delay(500);
                float voltage = readVoltage();
                float soc = readSOC();

                if (soc > 100 || soc < 40) {
                  sendJSON["error"] = "Floating VBAT Terminal...";
                  sendJSON["Result"] = "FAIL";
                } else {
                  if (voltage > 3.2 && voltage < 4.3) {
                    sendJSON["Result"] = "OK";
                  } else {
                    sendJSON["Result"] = "FAIL";  // Voltaje fuera de rango
                  }
                }

                sendJSON["voltage"] = voltage;
                sendJSON["SOC"] = soc;
              } else {
                // Fallo abrupto durante la transacción (ej. desconexión en caliente)
                sendJSON["Result"] = "FAIL";
                sendJSON["error"] = "Transaction failed mid-process";
                sendJSON["voltage"] = 0.0;
                sendJSON["SOC"] = 0.0;
              }

            } else {
              // 3. Target no responde o el bus falló
              sendJSON["Result"] = "FAIL";
              sendJSON["error"] = "No device MAX17048 found";
              sendJSON["voltage"] = 0.0;
              sendJSON["SOC"] = 0.0;

              // SOLO disparamos la recuperación agresiva si el error sugiere
              // un bus trabado (ej. código 4 o 5 en ESP32).
              // Si es código 2 (NACK), el bus está sano, solo falta el target.
              if (busStatus != 2) {
                recoverI2CBus();
              }
            }

            // 4. Respondemos siempre al frontend sin colgar el hilo
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:
          {
            sendJSON.clear();
            float sumaIn = 0.0;
            float sumaOut = 0.0;

            // Tomamos 10 lecturas espaciadas por 200 ms
            for (int i = 0; i < 10; i++) {
              sumaIn += abs(current_in());
              sumaOut += current_out();
              delay(200);
            }

            // Calculamos el promedio y aplicamos el offset al resultado final
            corrienteSensor = (sumaIn / 10.0) - 0.11;
            float corrienteOut = (sumaOut / 10.0);

            // Evaluación de rangos
            if (corrienteSensor > 0.180 && corrienteSensor < 0.25) {
              sendJSON["Result"] = "OK";
            } else {
              sendJSON["Result"] = "FAIL";  // Agregado: Es vital responder un FAIL explícito
            }

            // Formateo a 3 decimales fijos para evitar colas de basura flotante en el frontend
            sendJSON["currentSensor"] = String(corrienteSensor, 3) + " A";
            sendJSON["currentOut"] = String(corrienteOut, 3) + " A";

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  }
}
