#include <Wire.h>
#include <Adafruit_INA219.h>

#define I2C_SDA 6
#define I2C_SCL 7

TwoWire I2CBus = TwoWire(0);
Adafruit_INA219 ina219;

// Offset en vacío medido previamente (mV)
const float shuntOffset_mV = 0.0;  

// Resistencia shunt
const float R_SHUNT = 0.05;  // 50 mΩ

// Pines de botones
#define BTN1 20
#define BTN2 21

// Pines de relés (activo en LOW)
#define RELAY1 4
#define RELAY2 22
#define RELAY3 1
#define RELAY4 3

// Estados de los botones
bool estadoBtn1 = false;
bool estadoBtn2 = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  I2CBus.begin(I2C_SDA, I2C_SCL);

  if (!ina219.begin(&I2CBus)) {
    Serial.println("No se encontró el chip INA219");
    while (1) { delay(10); }
  }

  // Configurar botones con resistencia pull-up interna
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);

  // Configurar pines de relés como salida
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  // Apagar todos los relés al inicio (HIGH = apagado)
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);

  Serial.println("Midiendo voltaje y corriente con el INA219 ...");
}

void loop() {
  // ==== Lectura de botones ====
  if (digitalRead(BTN1) == LOW && !estadoBtn1) {
    estadoBtn1 = true;
    digitalWrite(RELAY1, LOW);   // Activo
    digitalWrite(RELAY2, LOW);
  } else if (digitalRead(BTN1) == HIGH && estadoBtn1) {
    estadoBtn1 = false;
    digitalWrite(RELAY1, HIGH);  // Apagado
    digitalWrite(RELAY2, HIGH);
  }

  if (digitalRead(BTN2) == LOW && !estadoBtn2) {
    estadoBtn2 = true;
    digitalWrite(RELAY3, LOW);   // Activo
    digitalWrite(RELAY4, LOW);
  } else if (digitalRead(BTN2) == HIGH && estadoBtn2) {
    estadoBtn2 = false;
    digitalWrite(RELAY3, HIGH);  // Apagado
    digitalWrite(RELAY4, HIGH);
  }

  // ==== Medición INA219 ====
  float shunt_mV = ina219.getShuntVoltage_mV();
  float bus_V = ina219.getBusVoltage_V();

  shunt_mV -= shuntOffset_mV;  
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT;
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;

  if (current_A > 3.5) {
    Serial.print("Corriente: ");
    Serial.print(current_A, 3);
    Serial.println(" A");
    //delay(2000);
 } 

  //delay(50); // un pequeño respiro para el loop
}
