#include <Wire.h>

#define MAX17048_ADDR 0x36
#define REG_VCELL     0x02
#define REG_SOC       0x04
#define REG_MODE      0x06 // Registro para comandos especiales

// Configura los pines I2C según tu Pulsar C6
const int I2C_SDA = 6;
const int I2C_SCL = 7;

void setup() {
  Serial.begin(115200);

  // Inicializamos el I2C con los pines específicos del ESP32-C6
  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.println("Iniciando comunicación con MAX17048...");
  delay(100); // Le damos tiempo al bus de estabilizarse
  
  // Opcional: Mandar un Quick-Start por defecto al iniciar el micro
  mandarQuickStart();
}

void loop() {
  float voltaje = leerVoltaje();
  float porcentaje = leerSOC();

  // Si el algoritmo se pierde por un pico de voltaje y da más del 100%
  if (porcentaje > 100.0) {
    Serial.println("-> SOC irreal detectado (>100%). Forzando Quick-Start...");
    mandarQuickStart();
    delay(500); // Esperamos medio segundo para que actualice los registros
    
    // Volvemos a tomar las lecturas ya corregidas
    voltaje = leerVoltaje();
    porcentaje = leerSOC();
  }

  Serial.print("Voltaje: ");
  Serial.print(voltaje, 3);  // 3 decimales de precisión
  Serial.print(" V  |  SOC: ");
  Serial.print(porcentaje, 1);
  Serial.println(" %");

  delay(2000);  // Leer cada 2 segundos
}

// Función para leer registros de 16 bits
uint16_t leerRegistro16(uint8_t reg) {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);  // Restart para mantener el bus

  Wire.requestFrom((uint8_t)MAX17048_ADDR, (size_t)2);

  if (Wire.available() == 2) {
    uint16_t msb = Wire.read();
    uint16_t lsb = Wire.read();
    return (msb << 8) | lsb;
  }
  return 0;
}

// Función para reiniciar el algoritmo del MAX17048
void mandarQuickStart() {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(REG_MODE);
  Wire.write(0x40); // MSB del comando 0x4000
  Wire.write(0x00); // LSB del comando 0x4000
  Wire.endTransmission();
}

float leerVoltaje() {
  uint16_t rawVcell = leerRegistro16(REG_VCELL);
  return rawVcell * 0.000078125;
}

float leerSOC() {
  uint16_t rawSoc = leerRegistro16(REG_SOC);
  return rawSoc / 256.0;
}