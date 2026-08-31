/*
Maincode test ue0098 con menu de interacción con PY32f003xx
Desarrollado por Esp32c6
*/

#include <Wire.h>
#include <ArduinoJson.h>


// ==== DECLARACIÓN DE GPIOS ====
#define I2C_SDA 6
#define I2C_SCL 7


// ==== REGISTROS DE MANIPULACIÓN ====
#define CMD_RELAY_OFF 0xA0
#define CMD_RELAY_ON 0xA1
#define CMD_ADC_PA0_12BIT 0x59
#define CMD_ADC_PA1_12BIT 0xDB

// ==== DECLARACIÓN DE VARIABLES GLOBALES ====
#define WIRE Wire
#define I2C_FREQ 100000  // 100 kHz BUS I2C
uint8_t found_devices[128];
uint8_t device_count = 0;
#define DEVICE_DELAY 30
constexpr uint32_t MIN_COMMAND_INTERVAL_MS = 25;
uint32_t last_i2c_command_time = 0;
bool i2c_command_sent = false;
bool i2c_safe_mode = false;
bool i2c_initialized = false;
String serial_line_buffer = "";

uint8_t transmitCommandByte(uint8_t address, uint8_t command);  // Declaraciones
bool sendCommand(uint8_t address, uint8_t command);
uint8_t parseHex(String hex_str);

// ==== CREACIÓN DE OBJETOS ====
StaticJsonDocument<1024> receiveJSON;
StaticJsonDocument<1024> sendJSON;



// ==== FUNCIONES DE UTILIDAD EN MANIPULACIÓN DE BUS I2C ====
bool waitForPinHigh(uint8_t pin, uint32_t timeout_us) {
  uint32_t started = micros();
  while (!digitalRead(pin)) {
    if ((uint32_t)(micros() - started) >= timeout_us) return false;
    delayMicroseconds(10);
  }
  return true;
}

bool clearI2CBus() {
  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delay(2);

  if (!waitForPinHigh(I2C_SCL, 20000)) return false;

  for (uint8_t pulse = 0; pulse < 18 && !digitalRead(I2C_SDA); pulse++) {
    digitalWrite(I2C_SCL, LOW);
    pinMode(I2C_SCL, OUTPUT);
    delayMicroseconds(5);
    pinMode(I2C_SCL, INPUT_PULLUP);
    if (!waitForPinHigh(I2C_SCL, 2000)) return false;
    delayMicroseconds(5);
  }

  digitalWrite(I2C_SCL, LOW);
  pinMode(I2C_SCL, OUTPUT);
  digitalWrite(I2C_SDA, LOW);
  pinMode(I2C_SDA, OUTPUT);
  delayMicroseconds(5);
  pinMode(I2C_SCL, INPUT_PULLUP);
  if (!waitForPinHigh(I2C_SCL, 2000)) {
    pinMode(I2C_SDA, INPUT_PULLUP);
    return false;
  }
  delayMicroseconds(5);
  pinMode(I2C_SDA, INPUT_PULLUP);
  delayMicroseconds(5);

  return digitalRead(I2C_SDA) && digitalRead(I2C_SCL);
}

bool initializeI2CBus(bool verbose) {
  if (i2c_initialized) {
    WIRE.end();
    i2c_initialized = false;
    delay(2);
  }

  bool was_blocked = !digitalRead(I2C_SDA) || !digitalRead(I2C_SCL);
  if (!clearI2CBus()) {
    i2c_safe_mode = true;
    if (verbose) Serial.println("[ERROR] Could not release the I2C bus");
    return false;
  }

  WIRE.begin(I2C_SDA, I2C_SCL);
  WIRE.setClock(I2C_FREQ);
  WIRE.setTimeout(50);

  i2c_initialized = true;
  i2c_safe_mode = false;
  i2c_command_sent = false;

  if (verbose) {
    Serial.println(was_blocked ? "[OK] Bus I2C recuperado e inicializado" : "[OK] I2C inicializado");
  }
  return true;
}

void serialDebug(String str) {
  StaticJsonDocument<255> doc;
  doc["debug"] = str;
  serializeJson(doc, Serial);
  Serial.println();
}

// ==== ====
void setup() {
  Serial.begin(115200);
  delay(500);
  sendJSON.clear();
  sendJSON["System"] = "Ready";
  sendJSON["Module"] = "TEMT6000 + PY32F003 DevLab";
  serializeJson(sendJSON, Serial);
  Serial.println();

  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delay(10);

  if (!initializeI2CBus(true)) {
    serialDebug("[SAFE MODE] Use 'recover' to retry connection with i2c bus");
  }

  /*
  Serial.println("\nComandos disponibles:");
  Serial.println("  scan     - Escanear bus I2C");
  Serial.println("  on XX    - Encender relay (XX = hex address)");
  Serial.println("  off XX   - Apagar relay");
  Serial.println("  adc0 XX  - Leer PA0/ADC_IN0 a 12 bits");
  Serial.println("  adc1 XX  - Leer PA1/ADC_IN1 a 12 bits");
  Serial.println("  recover  - Reiniciar bus I2C\n");
*/
}

void loop() {

  if (Serial.available()) {

    String JSONin = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSONin);

    if (error) {
      sendJSON.clear();
      sendJSON["status"] = "FAIL";
      sendJSON["error"] = String("Invalid JSON: ") + error.c_str();
      serializeJson(sendJSON, Serial);
      Serial.println();
    } else {

      String Function = receiveJSON["Function"];

      int opc = 0;

      if (Function == "ping") opc = 1;             // {"Function":"ping"}
      else if (Function == "scan") opc = 2;        // {"Function":"scan"}
      else if (Function == "onBlink") opc = 3;     // {"Function":"onBlink"}
      else if (Function == "offBlink") opc = 4;    // {"Function":"offBlink"}
      else if (Function == "readSensor") opc = 5;  // {"Function":"readSensor"}

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
            processCommand("scan");
            break;
          }

        case 3:
          {
            sendJSON.clear();
            processCommand("on 20");
            break;
          }

        case 4:
          {
            sendJSON.clear();
            processCommand("off 20");
            break;
          }

        case 5:
          {
            sendJSON.clear();
            processCommand("adc0 20");
            break;
          }
      }
    }
  }




  /*
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      serial_line_buffer.trim();
      if (serial_line_buffer.length() > 0) {
        String cmd = serial_line_buffer;
        cmd.toLowerCase();
        processCommand(cmd);
      }
      serial_line_buffer = "";
    } else if (serial_line_buffer.length() < 96) {
      serial_line_buffer += c;
    }
  }
*/
}




















// ═══════════════════════════════════════════════════════════
//             PROCESAMIENTO DE COMANDOS
// ═══════════════════════════════════════════════════════════
void processCommand(String cmd) {
  if (cmd == "scan" || cmd == "s") {
    scanDevices();
  } else if (cmd.startsWith("on ")) {
    relayOn(cmd);
  } else if (cmd.startsWith("off ")) {
    relayOff(cmd);
  } else if (cmd.startsWith("adc0 ")) {
    readADCCommand(cmd, false);
  } else if (cmd.startsWith("adc1 ")) {
    readADCCommand(cmd, true);
  } else if (cmd == "recover") {
    initializeI2CBus(true);
  } else {
    Serial.println("ERROR: Comando desconocido. Usa: scan, on XX, off XX, adc0 XX, adc1 XX");
  }
}

// ═══════════════════════════════════════════════════════════
//             FUNCIONES PRINCIPALES
// ═══════════════════════════════════════════════════════════
void scanDevices() {
  if (i2c_safe_mode) {
    Serial.println("[ERROR] I2C en SAFE MODE. Usa 'recover'.");
    return;
  }

  Serial.println("\n━━━ SCAN I2C ━━━");
  device_count = 0;
  WIRE.setTimeout(50);

  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    WIRE.beginTransmission(addr);
    if (WIRE.endTransmission() == 0) {
      found_devices[device_count++] = addr;
      Serial.printf("  [%02d] 0x%02X\n", device_count, addr);
      delay(DEVICE_DELAY);
    }
  }
  Serial.printf("━━━━━━━━━━━━━━━━\nTotal: %d devices\n\n", device_count);
}

void relayOn(String cmd) {
  uint8_t address = parseHex(cmd.substring(3));
  if (address == 0) return;

  if (sendCommand(address, CMD_RELAY_ON)) Serial.printf("[OK] Relay ON en 0x%02X\n", address);
  else Serial.printf("[FAIL] Fallo al encender relay en 0x%02X\n", address);
}

void relayOff(String cmd) {
  uint8_t address = parseHex(cmd.substring(4));
  if (address == 0) return;

  if (sendCommand(address, CMD_RELAY_OFF)) Serial.printf("[OK] Relay OFF en 0x%02X\n", address);
  else Serial.printf("[FAIL] Fallo al apagar relay en 0x%02X\n", address);
}

void readADCCommand(String cmd, bool adc1) {
  if (i2c_safe_mode) return;

  uint8_t address = parseHex(cmd.substring(5));
  if (address == 0) return;

  uint8_t adc_command = adc1 ? CMD_ADC_PA1_12BIT : CMD_ADC_PA0_12BIT;

  if (transmitCommandByte(address, adc_command) == 0) {
    delay(10);
    WIRE.setTimeout(100);
    if (WIRE.requestFrom(address, (uint8_t)2) == 2) {
      uint8_t hsb = WIRE.read();
      uint8_t lsb = WIRE.read();
      uint16_t adc_value = ((uint16_t)(hsb & 0x0F) << 8) | lsb;
      Serial.printf("[OK] 0x%02X - ADC%d: %u (0x%03X)\n", address, adc1 ? 1 : 0, adc_value, adc_value);
      return;
    }
    while (WIRE.available()) WIRE.read();  // Limpiar buffer en caso de lectura incompleta
  }
  Serial.printf("[FAIL] Fallo al leer ADC%d en 0x%02X\n", adc1 ? 1 : 0, address);
}

// ═══════════════════════════════════════════════════════════
//             UTILIDADES I2C
// ═══════════════════════════════════════════════════════════
uint8_t transmitCommandByte(uint8_t address, uint8_t command) {
  if (i2c_command_sent) {
    uint32_t elapsed = millis() - last_i2c_command_time;
    if (elapsed < MIN_COMMAND_INTERVAL_MS) delay(MIN_COMMAND_INTERVAL_MS - elapsed);
  }
  WIRE.beginTransmission(address);
  WIRE.write(command);
  uint8_t error = WIRE.endTransmission();
  last_i2c_command_time = millis();
  i2c_command_sent = true;
  return error;
}

bool sendCommand(uint8_t address, uint8_t command) {
  if (transmitCommandByte(address, command) != 0) return false;
  delay(20);
  WIRE.setTimeout(100);
  if (WIRE.requestFrom(address, (uint8_t)1) == 1) {
    WIRE.read();
    return true;
  }
  return false;
}

uint8_t parseHex(String hex_str) {
  hex_str.trim();
  char* endptr;
  long addr = strtol(hex_str.c_str(), &endptr, 16);
  if (*endptr != '\0' || addr < 0x08 || addr > 0x77) {
    Serial.println("ERROR: Dirección inválida (0x08-0x77)");
    return 0;
  }
  return (uint8_t)addr;
}