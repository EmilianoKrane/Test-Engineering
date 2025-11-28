// Código Final Integrado para LM2596 entre PagWeb y Pulsar ESP32-C6
// Código en el que estoy trabajando ahorita, acuerdateeeee 170925

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// Pines para UART2 (puedes reasignarlos)
#define RX2 D4  // GPIO15 como RX
#define TX2 D5  // GPIO19 como TX

// Pines para comunicación I2C con el sensor de corriente
#define I2C_SDA 6
#define I2C_SCL 7

// Relevador de puerto COM
#define RELAYCom 20

// Relevadores de Accionamiento de Fuente
#define RELAYA 8
#define RELAYB 9

// Relevadores de Inv Polaridad
#define RELAY1 21
#define RELAY2 1

// Relevadores de Cortocircuito
#define RELAY3 14
#define RELAY4 0

// Comunicación UART Creación Objeto
HardwareSerial PagWeb(1);  // Crear objeto para UART2 en PULSAR como PagWeb

// Comunicación I2C
TwoWire I2CBus = TwoWire(0);
Adafruit_INA219 ina219;

const float shuntOffset_mV = 0.0;  // Offset en vacío para lectura inicial
const float R_SHUNT = 0.05;        // Resistencia Shunt = 50 mΩ
float corrienteSensor = 0;         // Variable de lectura de corriente con el sensor

// JSON para recibir datos
String JSON;                        // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> datosJSON;  // Usa StaticJsonDocument para fijar tamaño

// JSON para enviar datos
String JSONCorriente;
StaticJsonDocument<200> enviarJSON;

void setup() {
  // Iniciar UART0 (USB) para depuración
  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");

  // Iniciar UART2 en los pines seleccionados
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("UART2 iniciado en RX=9, TX=8");

  // Iniciar comunicación I2C
  I2CBus.begin(I2C_SDA, I2C_SCL);

  if (!ina219.begin(&I2CBus)) {
    Serial.println("No se encontró el chip INA219");
    while (1) {
      delay(10);
    }
  }

  // Configurar pines de relés como salida
  pinMode(RELAYCom, OUTPUT);
  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  // Apagar todos los relés al inicio
  digitalWrite(RELAYCom, HIGH);
  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);

  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);

  Serial.println("Midiendo voltaje y corriente con el INA219 ...");
}

void loop() {
  //Serial.println(medirCorriente());
  //delay(100);

  //  Accionamiento desde PagWeb
  if (PagWeb.available()) {
    JSON = PagWeb.readStringUntil('\n');  // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(datosJSON, JSON);

    if (!error) {
      String Function = datosJSON["Function"];
      int opc = 0;

      // Llaves JSON de Ejecución de pruebas
      if (Function == "Fuente ON") opc = 5;
      else if (Function == "Fuente OFF") opc = 6;
      else if (Function == "COM ON") opc = 7;
      else if (Function == "COM OFF") opc = 8;
      else if (Function == "Carga Bat") opc = 9;

      switch (opc) {
        case 5:
          digitalWrite(RELAYA, LOW);
          digitalWrite(RELAYB, LOW);
          break;

        case 6:
          digitalWrite(RELAYA, HIGH);
          digitalWrite(RELAYB, HIGH);
          break;

        case 7:
          digitalWrite(RELAYCom, HIGH);
          break;

        case 8:
          digitalWrite(RELAYCom, LOW);
          break;

        case 9:
          enviarJSON.clear();
          digitalWrite(RELAYCom, HIGH);
          delay(200);

          bool state_lect1 = false;
          bool state_lect2 = false;

          // Medición negativa (mínima)
          float medicion_minima = 99999.0;
          for (int i = 0; i < 20; i++) {
            float medicion_actual = medirCorriente();
            Serial.print("Medición Neg ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(medicion_actual);

            if (medicion_actual < medicion_minima) {
              medicion_minima = medicion_actual;
            }
            delay(50);
          }


          Serial.print("Medición final Neg: ");
          Serial.println(medicion_minima);

          if (medicion_minima < -0.100 && medicion_minima > -0.270) {
            state_lect1 = true;
          }

          // Medición positiva (máxima)
          digitalWrite(RELAYCom, LOW);
          delay(200);

          float medicion_maxima = -99999.0;
          for (int i = 0; i < 20; i++) {
            float medicion_actual = medirCorriente();
            Serial.print("Medición Pos ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(medicion_actual);

            if (medicion_actual > medicion_maxima) {
              medicion_maxima = medicion_actual;
            }
            delay(50);
          }

          Serial.print("Medición final (mayor): ");
          Serial.println(medicion_maxima);

          if (medicion_maxima > 0.005) {
            state_lect2 = true;
          }

          // Envío JSON si cumple condiciones
          if (state_lect1 && state_lect2) {
            enviarJSON["BatteryChg"] = "OK";
            enviarJSON["MedNeg: "] = medicion_minima;
            enviarJSON["MedPos:"] = medicion_maxima;
            serializeJson(enviarJSON, PagWeb);
            PagWeb.println();
            Serial.println("Carga Bateria OK");
          } else {
            enviarJSON["BatteryChg"] = "F";
            enviarJSON["MedNeg: "] = medicion_minima;
            enviarJSON["MedPos:"] = medicion_maxima;
            serializeJson(enviarJSON, PagWeb);
            PagWeb.println();
            Serial.println("Carga Bateria FAIL");
          }

          digitalWrite(RELAYCom, HIGH);


          break;
      }
    } else {
      Serial.print("Error de JSON: ");
      Serial.println(error.c_str());
    }
  }  // ✅ cierre del if (PagWeb.available())
  else {
    // Falso contacto
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, HIGH);
    digitalWrite(RELAY3, HIGH);
    digitalWrite(RELAY4, HIGH);
  }
}  // ✅ cierre del loop completo

// ==== Medición INA219 ====
float medirCorriente() {
  float shunt_mV = ina219.getShuntVoltage_mV();
  float bus_V = ina219.getBusVoltage_V();
  shunt_mV -= shuntOffset_mV;
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT;
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;
  return current_A;
}
