/*

*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// ==== DECLARACIÓN DE GPIOS ====
#define SDA_PIN 6  //>> GPIO06 Bus de Datos SDA en I2C
#define SCL_PIN 7  //>> GPIO07 Señal de Reloj SCL en I2C
#define A0_PIN 4   //>> GPIO19 Control de Dirección A0 en FRAM
#define A1_PIN 5   //>> GPIO20 Control de Dirección A1 en FRAM
#define A2_PIN D0  //>> GPIO21 Control de Dirección A2 en FRAM

// ==== DECLARACIÓN DE VARIABLE GLOBALES ====
#define FRAM_BASE 0x50
uint8_t framAddr = FRAM_BASE;
uint16_t addrFRAM = 0x0010;  // Dirección de Escritura en la FRAM

StaticJsonDocument<300> receiveJSON;
StaticJsonDocument<300> sendJSON;  // Objeto para construir el JSON de salida

// ==== FUNCIONES DE UTILIDAD ====.
bool framWrite(uint16_t addr, const uint8_t *data, size_t len) {
  Wire.beginTransmission(framAddr);
  Wire.write(addr >> 8);
  Wire.write(addr & 0xFF);
  Wire.write(data, len);
  return (Wire.endTransmission() == 0);
}

bool framRead(uint16_t addr, uint8_t *buf, size_t len) {
  Wire.beginTransmission(framAddr);
  Wire.write(addr >> 8);
  Wire.write(addr & 0xFF);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom((int)framAddr, (int)len) != (int)len) return false;

  for (size_t i = 0; i < len; i++) {
    buf[i] = Wire.read();
  }
  return true;
}

void i2cScan() {
  Serial.println("\nI2C scan:");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Device found at 0x%02X\n", addr);
      if (addr >= 0x50 && addr <= 0x57) {
        framAddr = addr;
      }
    }
  }
}

// ==== FUNCION PARA BORRAR FRAM ====
bool framErase(uint16_t startAddr, size_t length) {
  // Creamos un buffer lleno de ceros (máximo 30 bytes para no saturar I2C)
  uint8_t zeroBuffer[30] = { 0 };
  size_t bytesWritten = 0;

  while (bytesWritten < length) {
    // Calculamos cuánto falta por escribir
    size_t chunk = length - bytesWritten;
    // Si falta más que nuestro buffer, lo limitamos a 30
    if (chunk > sizeof(zeroBuffer)) {
      chunk = sizeof(zeroBuffer);
    }

    // Reutilizamos tu función framWrite para escribir los ceros
    if (!framWrite(startAddr + bytesWritten, zeroBuffer, chunk)) {
      return false;  // Error en la comunicación
    }

    bytesWritten += chunk;  // Avanzamos el contador
  }

  return true;  // Borrado exitoso
}

void serialDebug(String cmd) {
  StaticJsonDocument<200> debug;
  debug["debug"] = cmd;
  serializeJson(debug, Serial);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(100);
  serialDebug("FRAM Test Initialized...");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // ==== DECLARACIÓN DE ENTRADAS Y SALIDAS ====
  pinMode(A0_PIN, OUTPUT);
  pinMode(A1_PIN, OUTPUT);
  pinMode(A2_PIN, OUTPUT);
  digitalWrite(A0_PIN, LOW);
  digitalWrite(A1_PIN, LOW);
  digitalWrite(A2_PIN, LOW);
}

void loop() {

  if (Serial.available()) {

    String JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];
      byte Addr = receiveJSON["Addr"] | 0x50;
      const char *msg = receiveJSON["Msg"] | "Hola Mundo :D";
      int eraseLen = receiveJSON["Len"] | 32;

      int opc = 0;
      if (Function == "ping") opc = 1;            // {"Function":"ping"};
      else if (Function == "scanAddr") opc = 2;   // {"Function":"address"}
      else if (Function == "readFRAM") opc = 3;   // {"Function":"readFRAM"}
      else if (Function == "writeFRAM") opc = 4;  // {"Function":"writeFRAM", "Msg":"Hola Mundo DevLab"}
      else if (Function == "eraseFRAM") opc = 5;  // {"Function":"eraseFRAM", "Len": 32}
      else if (Function == "testAll") opc = 6;    // {"Function":"testAll"}
                                                  // Response: {"state":"OK","write":true,"read":true,"data":"Hola Mundo :D"}

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
            serialDebug("Escaneando addr 0x50 - 0x57...");

            JsonArray arr = sendJSON.createNestedArray("addr");

            for (int i = 0; i < 8; i++) {

              int a0 = (i >> 0) & 1;
              int a1 = (i >> 1) & 1;
              int a2 = (i >> 2) & 1;

              digitalWrite(A0_PIN, a0);
              digitalWrite(A1_PIN, a1);
              digitalWrite(A2_PIN, a2);
              delay(5);

              uint8_t addr = 0x50 | (a2 << 2) | (a1 << 1) | a0;

              Wire.beginTransmission(addr);
              if (Wire.endTransmission() == 0) {

                String hexAddr = "0x" + String(addr, HEX);
                hexAddr.toUpperCase();

                serialDebug(hexAddr + " (EEPROM encontrada)");
                arr.add(hexAddr);
              }
            }

            if (arr.size() > 0) {
              framAddr = strtol(arr[0], NULL, 16);
              digitalWrite(A0_PIN, LOW);
              digitalWrite(A1_PIN, LOW);
              digitalWrite(A2_PIN, LOW);
            }

            if (arr.size() == 8) {
              sendJSON["Result"] = "OK";
            }

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:
          {
            sendJSON.clear();
            uint8_t buffer[32] = { 0 };

            framRead(addrFRAM, buffer, strlen(msg));
            Serial.print("Read: ");
            Serial.println((char *)buffer);
            break;
          }

        case 4:
          {
            sendJSON.clear();
            framWrite(addrFRAM, (const uint8_t *)msg, strlen(msg));
            break;
          }

        case 5:
          {
            sendJSON.clear();

            // Leemos cuántos bytes quiere borrar el usuario (por defecto 32 si no se envía)


            if (framErase(addrFRAM, eraseLen)) {
              serialDebug("FRAM borrada con éxito: " + String(eraseLen) + " bytes desde la dir 0x" + String(addrFRAM, HEX));
            } else {
              serialDebug("Error I2C al intentar borrar la FRAM");
            }
            break;
          }

        case 6:
          {
            sendJSON.clear();
            uint8_t buffer[32] = { 0 };
            String state = "FAIL";
            bool write = false;
            bool read = false;

            // ---- Borramos la memoria ----
            framErase(addrFRAM, eraseLen);
            delay(10);
            // ---- Escritura en memoria ----
            write = framWrite(addrFRAM, (const uint8_t *)msg, strlen(msg));
            delay(10);
            // ---- Lectura de memoria ----
            read = framRead(addrFRAM, buffer, strlen(msg));
            delay(10);
            if (read && write) state = "OK";
            sendJSON["Result"] = "OK";
            sendJSON["state"] = state;
            sendJSON["write"] = write;
            sendJSON["read"] = read;
            sendJSON["data"] = (char *)buffer;

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        default: break;
      }
    }
  }
}