/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL PWM JSON ====
*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// ==== Declaración de pines
#define RX2 D4  // GPIO15 como RX
#define TX2 D5  // GPIO19 como TX

#define I2C_SDA 22  // Sensor de corriente por I2C
#define I2C_SCL 23

#define RELAYA 8  // Accionamiento de Rele Fuente
#define RELAYB 9

#define RELAYPWM1 D0  // Accionamiento de Rele de conmutación PWM
#define RELAYPWM2 D1

int PWM_1 = 6;  // PWM pin 1
int PWM_2 = 7;  // PWM pin 2


// ==== Inicialización de objetos
HardwareSerial PagWeb(1);     // Objeto para UART2 en PULSAR como PagWeb
TwoWire I2CBus = TwoWire(0);  // Sensor de corriente
Adafruit_INA219 ina219;


// ==== Variables de inicialización
const float shuntOffset_mV = 0.0;  // Offset en vacío para lectura inicial
const float R_SHUNT = 0.05;        // Resistencia Shunt = 50 mΩ
float corrienteSensor = 0;         // Variable de lectura de corriente con el sensor

String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_corriente;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

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
    while (1) { delay(10); }
  }

  pinMode(RELAYA, OUTPUT);  // Relé de conmutación de fuente
  pinMode(RELAYB, OUTPUT);
  pinMode(RELAYPWM1, OUTPUT);  // Rele de conmutación de PWM
  pinMode(RELAYPWM2, OUTPUT);
  pinMode(PWM_1, OUTPUT);  // Salida de PWM
  pinMode(PWM_2, OUTPUT);


  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);
  digitalWrite(RELAYPWM1, HIGH);
  digitalWrite(RELAYPWM2, HIGH);

  Serial.println("Inicio del programa...");
  Serial.println("Midiendo corriente con INA219...");
}

void loop() {

  analogWrite(PWM_1, 255);
  analogWrite(PWM_2, 255);


  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {

      float medicion = 0;  // Variable para mediciones
      int STEP = 85;       // Paso del PWM

      String Function = receiveJSON["Function"];  // Function es la variable de interés del JSON

      int opc = 0;

      if (Function == "PWM_1") opc = 1;
      else if (Function == "PWM_2") opc = 2;

      switch (opc) {

        case 1:
          {
            bool status1 = true;
            float lastAverage = -1;

            Serial.println("Ejecución de PWM 1");
            Serial.println("");
            sendJSON.clear();  // Limpia cualquier dato previo

            digitalWrite(RELAYPWM1, HIGH);  //
            digitalWrite(RELAYPWM2, HIGH);

            // Fade up
            for (int duty = 255; duty >= 0; duty -= STEP) {
              analogWrite(PWM_1, duty);
              analogWrite(PWM_2, duty);
              Serial.print("Duty: ");
              Serial.println(duty);

              medicion = 0;
              delay(50);
              for (int i = 0; i < 10; i++) {
                medicion += medirCorriente();
              }

              float average = medicion / 10;

              // ---- VALIDACIÓN DE CRECIMIENTO ----
              if (lastAverage < 0) {
                lastAverage = average;
              } else {
                if (average > lastAverage + 0.5) {
                  // Sí está creciendo → OK
                  status1 = status1 && true;
                } else {
                  // No creció → error
                  status1 = false;
                }
                lastAverage = average;  // Actualización de estado
              }

              JSON_corriente = String(average, 3) + " A";  // Empaquetamiento
              Serial.print("El valor de corriente es: ");
              Serial.println(JSON_corriente);
              Serial.println("");

              String pack = "Duty" + String(duty);
              sendJSON[pack] = JSON_corriente;  // Envio de corriente JSON para corto

              delay(1000);
            }

            if (status1) {
              sendJSON["status"] = "OK";
            } else {
              sendJSON["status"] = "Fail";
            }

            serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
            PagWeb.println();

            break;
          }

        case 2:
          {
            bool status1 = true;
            float lastAverage = -1;

            Serial.println("Ejecución de PWM 1");
            Serial.println("");
            sendJSON.clear();  // Limpia cualquier dato previo

            digitalWrite(RELAYPWM1, LOW);  //
            digitalWrite(RELAYPWM2, LOW);
            delay(500);

            // Fade up
            for (int duty = 255; duty >= 0; duty -= STEP) {
              analogWrite(PWM_1, duty);
              analogWrite(PWM_2, duty);
              Serial.print("Duty: ");
              Serial.println(duty);

              medicion = 0;
              delay(50);
              for (int i = 0; i < 10; i++) {
                medicion += medirCorriente();
              }

              float average = medicion / 10;

              // ---- VALIDACIÓN DE CRECIMIENTO ----
              if (lastAverage < 0) {
                lastAverage = average;
              } else {
                if (average > lastAverage + 0.5) {
                  // Sí está creciendo → OK
                  status1 = status1 && true;
                } else {
                  // No creció → error
                  status1 = false;
                }
                lastAverage = average;  // Actualización de estado
              }

              JSON_corriente = String(average, 3) + " A";  // Empaquetamiento
              Serial.print("El valor de corriente es: ");
              Serial.println(JSON_corriente);
              Serial.println("");

              String pack = "Duty" + String(duty);
              sendJSON[pack] = JSON_corriente;  // Envio de corriente JSON para corto

              delay(1000);
            }

            if (status1) {
              sendJSON["status"] = "OK";
            } else {
              sendJSON["status"] = "Fail";
            }

            serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
            PagWeb.println();

            digitalWrite(RELAYPWM1, HIGH);  //
            digitalWrite(RELAYPWM2, HIGH);

            break;
          }
      }
    }
  }
}



// ==== Medición INA219 ====
float medirCorriente() {

  float shunt_mV = ina219.getShuntVoltage_mV();
  float bus_V = ina219.getBusVoltage_V();

  shunt_mV -= shuntOffset_mV;
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT;  // Corriente de interés
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;

  return current_A;
}
