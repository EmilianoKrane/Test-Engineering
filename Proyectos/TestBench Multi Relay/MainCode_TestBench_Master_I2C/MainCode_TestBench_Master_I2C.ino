#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <HardwareSerial.h>

#define SDA_PIN 6  // SDA para I2C con el esclavo
#define SCL_PIN 7  // SCL para I2C con el esclavo

// --- Dirección I2C base del esclavo ---
const uint8_t SLAVE_ADDR = 0x40; 

void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas para JSON válido
  Serial.println("{\"debug\": \"" + str + "\"}");
}

// --- Función para enviar comando I2C al esclavo ---
void sendCommandI2C(uint8_t command) {
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(command);
  uint8_t error = Wire.endTransmission();

  if (error == 0) {
    serialDebug("Comando enviado OK: 0x" + String(command, HEX));
  } else {
    serialDebug("Error I2C " + String(error) + " enviando: 0x" + String(command, HEX));
  }
}

// --- Función para leer respuesta del esclavo ---
uint8_t readResponseI2C() {
  Wire.requestFrom(SLAVE_ADDR, (uint8_t)1); // Solicitar 1 byte
  
  if (Wire.available()) {
    uint8_t response = Wire.read();
    serialDebug("Respuesta recibida: 0x" + String(response, HEX));
    return response;
  } else {
    serialDebug("No hubo respuesta del esclavo");
    return 0xFF; // Valor de error arbitrario
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  serialDebug("Serial Initialized...");

  Wire.begin(SDA_PIN, SCL_PIN);
  serialDebug("I2C Maestro inicializado en SDA: " + String(SDA_PIN) + " SCL: " + String(SCL_PIN));
  
  // Pequeña pausa para asegurar que el esclavo esté listo
  delay(500); 
}

void loop() {
  // Ejemplo de secuencia de prueba:
  
  // 1. Activar canal 3 (0x03)
  serialDebug("Activando canal 3...");
  sendCommandI2C(0x03);
  delay(50); // Pequeña pausa para que el esclavo procese
  readResponseI2C();
  delay(2000); 

  // 2. Activar canal 14 (0x0E)
  serialDebug("Activando canal 14...");
  sendCommandI2C(0x0E);
  delay(50);
  readResponseI2C();
  delay(2000);

  // 3. Apagar todo (Comando 0xFE configurado en tu esclavo)
  serialDebug("Modo reposo...");
  sendCommandI2C(0xFE);
  delay(50);
  readResponseI2C();
  delay(3000);
  
  // 4. Barrido (Comando 0xFF configurado en tu esclavo)
  // Nota: El barrido dura unos 3.2 segundos (16 * 0.2s), así que no 
  // leas la respuesta inmediatamente.
  serialDebug("Iniciando barrido de prueba...");
  sendCommandI2C(0xFF);
  delay(4000); // Esperar a que termine el barrido
  readResponseI2C();
  delay(3000);
}