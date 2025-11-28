#include <Wire.h>
#include <Adafruit_INA219.h>

#define I2C_SDA 6
#define I2C_SCL 7

TwoWire I2CBus = TwoWire(0);
Adafruit_INA219 ina219;

// Offset en vacío medido previamente (mV)
const float shuntOffset_mV = 0.0;
const float R_SHUNT = 0.05;  // 50 mΩ

// Relevadores de Accionamiento de Fuente
#define RELAYA 8
#define RELAYB 9

void setup() {
  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");

  I2CBus.begin(I2C_SDA, I2C_SCL);

  if (!ina219.begin(&I2CBus)) {
    Serial.println("No se encontró el chip INA219");
    while (1) { delay(10); }
  }


  // Configurar pines de relés como salida
  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);

  // Apagar todos los relés al inicio (HIGH = apagado)
  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);

  Serial.println("Midiendo voltaje y corriente con el INA219 ...");
}

void loop() {

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
