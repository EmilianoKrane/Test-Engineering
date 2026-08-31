// ═══════════════════════════════════════════════════════════
// ADMIN SIMPLE - Controlador I2C Simplificado (Scan, Relay, ADC)
// ═══════════════════════════════════════════════════════════

#include <Wire.h>

// Configuración I2C seleccionada automáticamente por arquitectura.
#if defined(ARDUINO_ARCH_RP2040)
  #define WIRE Wire1
  #ifndef DEVLAB_I2C_SDA
    #define DEVLAB_I2C_SDA 12
  #endif
  #ifndef DEVLAB_I2C_SCL
    #define DEVLAB_I2C_SCL 13
  #endif
  #define I2C_BUS_NAME "Wire1"
#elif defined(ARDUINO_ARCH_ESP32)
  #define WIRE Wire
  #ifndef DEVLAB_I2C_SDA
    #define DEVLAB_I2C_SDA 6
  #endif
  #ifndef DEVLAB_I2C_SCL
    #define DEVLAB_I2C_SCL 7
  #endif
  #define I2C_BUS_NAME "Wire"
#else
  #error "Unsupported architecture: use RP2040/RP2350 or ESP32"
#endif
#define I2C_SDA DEVLAB_I2C_SDA
#define I2C_SCL DEVLAB_I2C_SCL
#define I2C_FREQ 100000  // 100 kHz

// ═══════════════════════════════════════════════════════════
//             COMANDOS HABILITADOS
// ═══════════════════════════════════════════════════════════
#define CMD_RELAY_OFF      0xA0   
#define CMD_RELAY_ON       0xA1   
#define CMD_ADC_PA0_12BIT  0x59
#define CMD_ADC_PA1_12BIT  0xDB

// Variables globales
uint8_t found_devices[128];
uint8_t device_count = 0;
#define DEVICE_DELAY 30          
constexpr uint32_t MIN_COMMAND_INTERVAL_MS = 25;
uint32_t last_i2c_command_time = 0;
bool i2c_command_sent = false;
bool i2c_safe_mode = false;
bool i2c_initialized = false;
String serial_line_buffer = "";

// Declaraciones
uint8_t transmitCommandByte(uint8_t address, uint8_t command);
bool sendCommand(uint8_t address, uint8_t command);
uint8_t parseHex(String hex_str);

// ═══════════════════════════════════════════════════════════
//             RECUPERACIÓN E INICIALIZACIÓN I2C
// ═══════════════════════════════════════════════════════════
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

  #if defined(ARDUINO_ARCH_RP2040)
    WIRE.setSDA(I2C_SDA);
    WIRE.setSCL(I2C_SCL);
    WIRE.begin();
  #elif defined(ARDUINO_ARCH_ESP32)
    WIRE.begin(I2C_SDA, I2C_SCL);
  #endif
  
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

// ═══════════════════════════════════════════════════════════
//             SETUP Y LOOP
// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  #if defined(ARDUINO_ARCH_RP2040)
    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 2000) delay(10);
    delay(200);
  #else
    delay(500);
  #endif

  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║   ADMIN - I2C Control (Simplificado)  ║");
  Serial.println("╚═══════════════════════════════════════╝");
  Serial.println("[INFO] Inicializando I2C...");

  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delay(10);

  if (!initializeI2CBus(true)) {
    Serial.println("[SAFE MODE] Use 'recover' to retry");
  }

  Serial.println("\nComandos disponibles:");
  Serial.println("  scan     - Escanear bus I2C");
  Serial.println("  on XX    - Encender relay (XX = hex address)");
  Serial.println("  off XX   - Apagar relay");
  Serial.println("  adc0 XX  - Leer PA0/ADC_IN0 a 12 bits");
  Serial.println("  adc1 XX  - Leer PA1/ADC_IN1 a 12 bits");
  Serial.println("  recover  - Reiniciar bus I2C\n");
}

void loop() {
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
    while (WIRE.available()) WIRE.read(); // Limpiar buffer en caso de lectura incompleta
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