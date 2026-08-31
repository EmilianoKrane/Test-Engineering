// ═══════════════════════════════════════════════════════════
// ADMIN SIMPLE - Controlador universal legacy I2C
// Universal legacy I2C control console
// ═══════════════════════════════════════════════════════════

#include <Wire.h>
#include <ArduinoJson.h>

// I2C configuration selected automatically by architecture.
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
#define I2C_FREQ 100000  // 100 kHz - safe for initialization (can be raised to 400 kHz later)

// ═══════════════════════════════════════════════════════════
//           CRITICAL SAFETY COMMANDS (0xA0-0xAF)
// ═══════════════════════════════════════════════════════════
#define CMD_RELAY_OFF 0xA0        // Turn relay output off
#define CMD_RELAY_ON 0xA1         // Turn relay output on
#define CMD_RELAY_TOGGLE 0xA6     // Relay pulse/trigger
#define CMD_SET_TOGGLE_TIME 0xA7  // Configure relay pulse time in 25 ms blocks
#define CMD_GET_TOGGLE_TIME 0xA8  // Read relay pulse time in 25 ms blocks

// ═══════════════════════════════════════════════════════════
//           RGB LED COMMANDS
// ═══════════════════════════════════════════════════════════
#define CMD_RED 0x02    // Apply slot 1
#define CMD_GREEN 0x03  // Apply slot 2
#define CMD_BLUE 0x04   // Apply slot 3
#define CMD_OFF 0x05    // RGB LED off + PWM off
#define CMD_WHITE 0x08  // RGB LED white

// ═══════════════════════════════════════════════════════════
//           PWM / BUZZER COMMANDS
// ═══════════════════════════════════════════════════════════
#define CMD_PWM_OFF 0x20  // PWM OFF - Silence
#define CMD_PWM_25 0x21   // PWM 25% - 200Hz
#define CMD_PWM_50 0x22   // PWM 50% - 500Hz
#define CMD_PWM_75 0x23   // PWM 75% - 1000Hz
#define CMD_PWM_100 0x24  // PWM 100% - 2000Hz

// ═══════════════════════════════════════════════════════════
//           DIGITAL INPUT COMMANDS
// ═══════════════════════════════════════════════════════════
#define CMD_PA4_DIGITAL 0x07   // Read digital input
#define CMD_PA0_DIGITAL 0x09   // Read digital PA0 on the joystick I2C slave
#define RESP_PA4_DIGITAL 0x09  // Digital PA4 response

// ═══════════════════════════════════════════════════════════
//           12-BIT ADC COMMANDS
// ═══════════════════════════════════════════════════════════
#define CMD_ADC_PA0_HSB 0x56
#define CMD_ADC_PA0_LSB 0x57
#define CMD_ADC_PA0_I2C 0x58
#define CMD_ADC_PA0_12BIT 0x59
#define CMD_ADC_PA1_HSB 0xD8
#define CMD_ADC_PA1_LSB 0xD9
#define CMD_ADC_PA1_I2C 0xDA
#define CMD_ADC_PA1_12BIT 0xDB
#define CMD_SET_ADC_AVERAGING 0xDC
#define CMD_GET_ADC_AVERAGING 0xDD

// ═══════════════════════════════════════════════════════════
//           I2C ADDRESS MANAGEMENT COMMANDS
// ═══════════════════════════════════════════════════════════
#define CMD_SET_I2C_ADDR 0x3D    // Set a new I2C address (stored in Flash)
#define CMD_RESET_FACTORY 0x3E   // Factory reset (use UID by default)
#define CMD_GET_I2C_STATUS 0x3F  // Get I2C state (Flash vs UID)
#define CMD_SAVE_COLOR 0x46      // Store RGB in slot (1..3 in admin, 0..2 in slave)

#define RESP_I2C_ADDR_SET 0x0D       // New I2C address set
#define RESP_FACTORY_RESET 0x0E      // Factory reset completed (use UID)
#define RESP_I2C_FROM_FLASH 0x0F     // I2C using address from Flash
#define RESP_I2C_FROM_UID 0x0A       // I2C using address from UID
#define RESP_COLOR_STAGE_READY 0x15  // save_slot stage ready
#define RESP_COLOR_SAVED 0x16        // Slot stored in Flash

// ═══════════════════════════════════════════════════════════
//           PERSISTENT RGB REGISTERS
// ═══════════════════════════════════════════════════════════
#define REG_RGB_RED 0x60        // Red register (0x00-0xFF)
#define REG_RGB_BLUE 0x62       // Blue register (0x00-0xFF)
#define REG_RGB_GREEN 0x64      // Green register (0x00-0xFF)
#define REG_RGB_OFF 0x06        // Reserved for compatibility with the slave legacy command
#define REG_RGB_OFF_VALUE 0x00  // Off value (not used in register mode)

// Variables globales
uint8_t found_devices[128];
uint8_t device_count = 0;
bool scan_loop_active = false;  // Continuous scan flag
uint32_t last_scan_time = 0;    // Timestamp of the last scan
uint32_t scan_interval = 1000;  // Interval between scans (1 second)
bool scan_in_progress = false;  // Flag that prevents simultaneous scans
#define DEVICE_DELAY 30         // Optimal delay between devices (ms)
constexpr uint32_t MIN_COMMAND_INTERVAL_MS = 25;
// One pacing clock for the entire bus; it also spaces commands sent to different nodes.
uint32_t last_i2c_command_time = 0;
bool i2c_command_sent = false;

uint8_t transmitCommandByte(uint8_t address, uint8_t command);

struct DeviceError {
  uint8_t address;
  uint32_t error_count;
  uint32_t success_count;
  uint32_t last_error_time;
};
DeviceError device_errors[128];
int device_error_count = 0;

// Test mode
bool test_mode_active = false;
uint32_t test_interval = 100;  // Test interval (100 ms by default)
uint32_t test_start_time = 0;
uint32_t test_duration = 0;

// Digital input reading stats
struct DigitalInputStats {
  uint8_t address;
  uint32_t read_count;
  uint32_t fail_count;
  uint8_t last_state;  // 0=LOW, 1=HIGH
  uint32_t last_read_time;
};
DigitalInputStats digital_stats[128];
int digital_stats_count = 0;

// Test mode for digital reads
bool read_test_active = false;
uint32_t read_test_interval = 100;
uint32_t read_test_start_time = 0;
uint32_t read_test_duration = 0;
uint8_t last_digital_input_pin = 4;

bool autoread_active = false;
uint32_t autoread_interval = 2000;  // 2 seconds by default
uint32_t last_autoread_time = 0;
uint8_t autoread_address = 0;

// Flag used to reprint the menu when Serial connects
bool menu_shown = false;
uint32_t last_serial_check = 0;
bool i2c_safe_mode = false;  // Safe mode when I2C is blocked
bool i2c_initialized = false;
String serial_line_buffer = "";
String deferred_serial_cmd = "";
bool deferred_serial_cmd_ready = false;

bool waitForPinHigh(uint8_t pin, uint32_t timeout_us) {
  uint32_t started = micros();
  while (!digitalRead(pin)) {
    if ((uint32_t)(micros() - started) >= timeout_us) {
      return false;
    }
    delayMicroseconds(10);
  }
  return true;
}

// Release a slave left in the middle of a transaction when the master
// was reset. Pins are only driven LOW; pull-ups generate HIGH.
bool clearI2CBus() {
  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delay(2);

  if (!waitForPinHigh(I2C_SCL, 20000)) {
    return false;
  }

  // Up to two groups of 9 pulses: complete the pending byte and force NACK.
  for (uint8_t pulse = 0; pulse < 18 && !digitalRead(I2C_SDA); pulse++) {
    digitalWrite(I2C_SCL, LOW);
    pinMode(I2C_SCL, OUTPUT);
    delayMicroseconds(5);
    pinMode(I2C_SCL, INPUT_PULLUP);
    if (!waitForPinHigh(I2C_SCL, 2000)) {
      return false;
    }
    delayMicroseconds(5);
  }

  // Generate a STOP condition regardless of the previous SDA state.
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
    if (verbose) {
      Serial.println("[ERROR] Could not release the I2C bus");
      Serial.println("        Check for shorts, power, and pull-up resistors");
    }
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
    Serial.println(was_blocked ? "[OK] Bus I2C recuperado e inicializado"
                               : "[OK] I2C inicializado");
    Serial.printf("     %s - SDA:GPIO%d SCL:GPIO%d @ %d kHz\n",
                  I2C_BUS_NAME, I2C_SDA, I2C_SCL, I2C_FREQ / 1000);
  }
  return true;
}

// ==== Variables de inicialización JSON
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;


void setup() {
  // ═══════════════════════════════════════════════════════════
  // STEP 1: INITIALIZE USB/CDC FIRST (CRITICAL ON RP2040/RP2350)
  // ═══════════════════════════════════════════════════════════
  Serial.begin(115200);


#if defined(ARDUINO_ARCH_RP2040)
  // RP2040/RP2350: Wait for USB enumeration BEFORE touching I2C
  // This prevents I2C from blocking IRQs and stopping CDC
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2000) {
    delay(10);
  }
  delay(200);  // Extra safety margin

  Serial.println("\n[RP2040/RP2350] USB CDC listo");
#else
  delay(500);
#endif

  Serial.println("\n\n");
  Serial.println("╔═══════════════════════════════════════╗");
  Serial.println("║   ADMIN - I2C Control                ║");
  Serial.println("║   Scan + Relay + PWM + RGB Registers ║");
  Serial.println("╚═══════════════════════════════════════╝");

  // ═══════════════════════════════════════════════════════════
  // STEP 2: INITIALIZE I2C AFTER USB IS ACTIVE
  // ═══════════════════════════════════════════════════════════
  Serial.println("\n[INFO] Inicializando I2C...");

  // Check that SDA/SCL are not blocked BEFORE WIRE.begin()
  Serial.println("[CHECK] Verificando bus I2C...");
  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delay(10);

  bool sda_ok = digitalRead(I2C_SDA);
  bool scl_ok = digitalRead(I2C_SCL);

  Serial.printf("  SDA (GPIO%d): %s\n", I2C_SDA, sda_ok ? "HIGH ✓" : "LOW ✗ BLOCKED");
  Serial.printf("  SCL (GPIO%d): %s\n", I2C_SCL, scl_ok ? "HIGH ✓" : "LOW ✗ BLOCKED");

  if (!sda_ok || !scl_ok) {
    Serial.println("[WARN] Bus busy; attempting clock + STOP recovery...");
  } else {
    Serial.println("[OK] Bus I2C libre");
  }

  if (!initializeI2CBus(true)) {
    Serial.println("[SAFE MODE] Use 'recover' to retry without resetting");
  }

  Serial.println("\nComandos disponibles:");
  Serial.println("  s              - Scan I2C devices");
  Serial.println("  loop           - Continuous scan (every second)");
  Serial.println("  stop           - Stop continuous scan");
  Serial.println("  recover        - Release and reinitialize the I2C bus");
  Serial.println("  t XX           - Relay pulse (XX = hex address)");
  Serial.println("  pulse XX BB    - Change relay pulse time: hex block 01..28");
  Serial.println("                   Time = BB * 25 ms");
  Serial.println("  time XX BB     - Alias for pulse");
  Serial.println("  gettime XX     - Read relay pulse time");
  Serial.println("  adc0 XX        - Read PA0/ADC_IN0 at 12 bits");
  Serial.println("  adc1 XX        - Read PA1/ADC_IN1 at 12 bits");
  Serial.println("  average XX N   - Set ADC averaging (N = 4, 8, 16 or 24)");
  Serial.println("  getaverage XX  - Read ADC averaging sample count");
  Serial.println("  on XX          - Turn relay on");
  Serial.println("  off XX         - Turn relay off");
  Serial.println("  wr A R V       - Write register (addr reg value)");
  Serial.println("  rgb A R G B    - Set RGB in registers 0x60/0x64/0x62");
  Serial.println("  offrgb A       - Turn color off (RGB=0 + CMD_OFF)");
  Serial.println("  slot XX N      - Apply slot LED RGB (N=1..3)");
  Serial.println("  save_slot A N R G B - Store RGB in slot (N=1..3)");
  Serial.println("  help           - Show help");
  Serial.println("\nEjemplos:");
  Serial.println("  s              (device scan)");
  Serial.println("  loop           (scan continuo)");
  Serial.println("  stop           (stop scan)");
  Serial.println("  t 32           (toggle relay at 0x20)");
  Serial.println("  pulse 32 05    (125 ms relay pulse)");
  Serial.println("  pulse 32 28    (1 second relay pulse)");
  Serial.println("  adc0 32        (read PA0/ADC_IN0)");
  Serial.println("  adc1 32        (read PA1/ADC_IN1)");
  Serial.println("  off 32         (turn relay off at 0x20)");
  Serial.println("  slot 20 1      (apply slot 1 at 0x20)");
  Serial.println("  save_slot 20 1 0x00 0xff 0x60");
  Serial.println("\n");
}

bool readSerialCommand(String* cmd) {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r' || c == '\n') {
      serial_line_buffer.trim();
      if (serial_line_buffer.length() > 0) {
        *cmd = serial_line_buffer;
        cmd->trim();
        cmd->toLowerCase();
        serial_line_buffer = "";
        return true;
      }
      serial_line_buffer = "";
    } else if (serial_line_buffer.length() < 96) {
      serial_line_buffer += c;
    } else {
      serial_line_buffer = "";
      Serial.println("[WARN] Serial command is too long; discarded");
    }
  }

  return false;
}

bool pollSerialCommands(bool scan_running) {
  bool stop_received = false;

  if (!scan_running && deferred_serial_cmd_ready) {
    String cmd = deferred_serial_cmd;
    deferred_serial_cmd = "";
    deferred_serial_cmd_ready = false;
    processCommand(cmd);
  }

  String cmd;
  while (readSerialCommand(&cmd)) {
    if (!scan_running) {
      processCommand(cmd);
    } else if (cmd == "stop") {
      stopScanLoop();
      stop_received = true;
    } else if (!deferred_serial_cmd_ready) {
      deferred_serial_cmd = cmd;
      deferred_serial_cmd_ready = true;
      Serial.println("[INFO] Scan in progress; command queued until it finishes");
    } else {
      Serial.println("[WARN] Scan in progress; command discarded because one is already queued");
    }
  }

  return stop_received;
}

void loop() {
// Reprint the menu when Serial Monitor reconnects
#if defined(ARDUINO_ARCH_RP2040)
  if (Serial && !menu_shown && millis() - last_serial_check > 1000) {
    showMenu();
    menu_shown = true;
    last_serial_check = millis();
  }
  if (!Serial) {
    menu_shown = false;
    last_serial_check = millis();
  }
#endif

  // Process Serial commands without blocking on a timeout.
  pollSerialCommands(false);

  // Run continuous scan when active
  if (scan_loop_active && !scan_in_progress) {
    if (millis() - last_scan_time >= scan_interval) {
      scanDevices();
      last_scan_time = millis();
    }
  }

  // Run the asynchronous test when active
  if (test_mode_active) {
    if (millis() - last_scan_time >= test_interval) {
      testDevices();
      last_scan_time = millis();
    }
    // Check whether the test should stop
    if (test_duration > 0 && (millis() - test_start_time >= test_duration)) {
      stopTest();
      showTestResults();
    }
  }

  // Run continuous autoread when active
  if (autoread_active) {
    if (millis() - last_autoread_time >= autoread_interval) {
      autoReadTick();
      last_autoread_time = millis();
    }
  }

  // Run the digital-read test when active
  if (read_test_active) {
    if (millis() - last_scan_time >= read_test_interval) {
      testDigitalInputReads();
      last_scan_time = millis();
    }
    // Check whether the test should stop
    if (read_test_duration > 0 && (millis() - read_test_start_time >= read_test_duration)) {
      stopReadTest();
    }
  }

  pollSerialCommands(false);
}

void serialDebug(String str) {
  sendJSON.clear();
  sendJSON["debug"] = str;
  serializeJson(sendJSON, Serial);
  Serial.println();
}

void processCommand(String cmd) {
  if (cmd == "s" || cmd == "scan") {
    scanDevices();
  } else if (cmd == "loop") {
    startScanLoop();
  } else if (cmd == "stop") {
    stopScanLoop();
    stopAutoRead();
  } else if (cmd == "recover" || cmd == "recovery") {
    initializeI2CBus(true);
  } else if (cmd == "menu" || cmd == "m") {
    showMenu();
  } else if (cmd.startsWith("test ")) {
    startTest(cmd);
  } else if (cmd == "stoptest") {
    stopTest();
    showTestResults();
  } else if (cmd == "errors") {
    showTestResults();
  } else if (cmd.startsWith("read ")) {
    readDigitalInputCommand(cmd);
  } else if (cmd.startsWith("read0 ")) {
    readPA0DigitalInputCommand(cmd);
  } else if (cmd == "dstats") {
    showDigitalInputStats();
  } else if (cmd.startsWith("readtest ")) {
    startReadTest(cmd);
  } else if (cmd == "stopread") {
    stopReadTest();
  } else if (cmd.startsWith("adc0 ")) {
    readADCCommand(cmd, false);
  } else if (cmd.startsWith("adc1 ")) {
    readADCCommand(cmd, true);
  } else if (cmd.startsWith("average ")) {
    setAdcAveraging(cmd);
  } else if (cmd.startsWith("getaverage ")) {
    getAdcAveraging(cmd);
  } else if (cmd.startsWith("t ")) {
    toggleRelay(cmd);
  } else if (cmd.startsWith("time ") || cmd.startsWith("pulse ") || cmd.startsWith("pulso ")) {
    setToggleTime(cmd);
  } else if (cmd.startsWith("gettime ")) {
    getToggleTime(cmd);
  } else if (cmd.startsWith("on ")) {
    relayOn(cmd);
  } else if (cmd.startsWith("off ")) {
    relayOff(cmd);
  } else if (cmd.startsWith("pwm ")) {
    pwmCommand(cmd);
  } else if (cmd == "silence" || cmd == "pwmoff") {
    pwmOff();
  } else if (cmd.startsWith("slot ") || cmd.startsWith("led ") || cmd.startsWith("neo ")) {
    slotCommand(cmd);
  } else if (cmd.startsWith("slot1 ")) {
    slotShortcutCommand(cmd, 1);
  } else if (cmd.startsWith("slot2 ")) {
    slotShortcutCommand(cmd, 2);
  } else if (cmd.startsWith("slot3 ")) {
    slotShortcutCommand(cmd, 3);
  } else if (cmd.startsWith("red ")) {
    slotShortcutCommand(cmd, 1);
  } else if (cmd.startsWith("green ")) {
    slotShortcutCommand(cmd, 2);
  } else if (cmd.startsWith("blue ")) {
    slotShortcutCommand(cmd, 3);
  } else if (cmd.startsWith("white ")) {
    neoWhite(cmd);
  } else if (cmd == "neooff" || cmd == "ledoff") {
    neoOff();
  } else if (cmd.startsWith("ch ")) {
    changeI2CAddress(cmd);
  } else if (cmd.startsWith("wr ")) {
    writeRegisterCommand(cmd);
  } else if (cmd.startsWith("rgb ")) {
    rgbRegisterCommand(cmd);
  } else if (cmd.startsWith("offrgb ")) {
    rgbOffRegisterCommand(cmd);
  } else if (cmd.startsWith("save_slot ") || cmd.startsWith("save_color ")) {
    saveSlotPreset(cmd);
  } else if (cmd.startsWith("sweepr ")) {
    redSweepCommand(cmd);
  } else if (cmd == "help" || cmd == "h") {
    showHelp();
  } else if (cmd.startsWith("autoread ")) {
    startAutoRead(cmd);
  } else if (cmd == "stopautoread") {
    stopAutoRead();
  } else {
    Serial.println("ERROR: Unknown command. Use 'help' or 'menu' to list commands.");
  }
}

void showMenu() {
  Serial.println("\n\n");
  Serial.println("╔═══════════════════════════════════════╗");
  Serial.println("║   ADMIN - I2C Control                ║");
  Serial.println("║   Scan + Relay + PWM + RGB Registers ║");
  Serial.println("╚═══════════════════════════════════════╝");
  Serial.println("[OK] I2C initialized without blocking USB");
  Serial.printf("     SDA:GPIO%d SCL:GPIO%d @ %d kHz\n", I2C_SDA, I2C_SCL, I2C_FREQ / 1000);
  Serial.println("\nComandos disponibles:");
  Serial.println("  s / menu       - Scan devices / show menu");
  Serial.println("  loop           - Continuous scan (every second)");
  Serial.println("  stop           - Stop continuous scan");
  Serial.println("  recover        - Release and reinitialize the I2C bus");
  Serial.println("  t XX           - Relay pulse (XX = hex address)");
  Serial.println("  pulse XX BB    - Change relay pulse time: hex block 01..28");
  Serial.println("                   Time = BB * 25 ms");
  Serial.println("  time XX BB     - Alias for pulse");
  Serial.println("  gettime XX     - Read relay pulse time");
  Serial.println("  adc0 XX        - Read PA0/ADC_IN0 at 12 bits");
  Serial.println("  adc1 XX        - Read PA1/ADC_IN1 at 12 bits");
  Serial.println("  average XX N   - Set ADC averaging (N = 4, 8, 16 or 24)");
  Serial.println("  getaverage XX  - Read ADC averaging sample count");
  Serial.println("  on XX          - Turn relay on");
  Serial.println("  off XX         - Turn relay off");
  Serial.println("  pwm XX NIVEL   - PWM (25, 50, 75, 100, off)");
  Serial.println("  silence        - Turn PWM off on all devices");
  Serial.println("  slot XX N      - Apply slot LED RGB (N=1..3)");
  Serial.println("  wr A R V       - Write I2C register (hex)");
  Serial.println("  rgb A R G B    - Write RGB color through registers");
  Serial.println("  offrgb A       - Turn color off (RGB=0 + CMD_OFF)");
  Serial.println("  save_slot A N R G B - Store RGB in slot (N=1..3)");
  Serial.println("  sweepr A D     - Red sweep (D=delay in ms, optional)");
  Serial.println("  ch XX YY       - Change I2C address (XX=old, YY=new)");
  Serial.println("  help           - Show full help");
  Serial.println("\nEjemplos:");
  Serial.println("  s              (device scan)");
  Serial.println("  menu           (show this menu)");
  Serial.println("  pulse 20 05    (125 ms relay pulse)");
  Serial.println("  pulse 20 28    (1 second relay pulse)");
  Serial.println("  adc0 20        (read PA0/ADC_IN0)");
  Serial.println("  adc1 20        (read PA1/ADC_IN1)");
  Serial.println("  average 20 16  (average 16 ADC samples)");
  Serial.println("  pwm 20 50      (PWM 50% at 0x20)");
  Serial.println("  silence        (turn PWM off on all devices)");
  Serial.println("  slot 20 1      (apply slot 1 at 0x20)");
  Serial.println("  wr 20 60 ff    (Register 0x60 = maximum red)");
  Serial.println("  rgb 20 ff 00 00 (Red through registers)");
  Serial.println("  offrgb 20      (turn off and store the OFF state)");
  Serial.println("  save_slot 20 1 0x00 0xff 0x60");
  Serial.println("");
}

void scanDevices() {
  // Check whether safe mode is active
  if (i2c_safe_mode) {
    Serial.println("[ERROR] I2C is in SAFE MODE - command unavailable");
    Serial.println("Use 'recover' to release the bus without resetting");
    return;
  }

  // Prevent simultaneous scans
  if (scan_in_progress) {
    Serial.println("⚠ Scan already in progress; waiting...");
    return;
  }

  scan_in_progress = true;
  Serial.println("\n━━━ SCAN I2C ━━━");

  device_count = 0;
  memset(found_devices, 0, sizeof(found_devices));

  WIRE.setTimeout(50);  // Short timeout for speed

  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    if (pollSerialCommands(true)) {
      break;
    }

    WIRE.beginTransmission(addr);
    uint8_t error = WIRE.endTransmission();

    if (error == 0) {
      found_devices[device_count++] = addr;
      Serial.printf("  [%02d] 0x%02X (%3d)\n", device_count, addr, addr);

      delay(DEVICE_DELAY);  // Delay optimizado: 30ms
    }
  }

  Serial.println("━━━━━━━━━━━━━━━━");
  Serial.printf("Total: %d devices\n\n", device_count);

  if (device_count == 0) {
    Serial.println("⚠ No I2C devices found");
    Serial.println("  Check connections and pull-ups\n");
  }

  scan_in_progress = false;
}

void toggleRelay(String cmd) {
  // Check whether safe mode is active
  if (i2c_safe_mode) {
    Serial.println("[ERROR] I2C is in SAFE MODE - command unavailable");
    return;
  }

  String addr_str = cmd.substring(2);
  addr_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  if (sendCommand(address, CMD_RELAY_TOGGLE)) {
    Serial.printf("[OK] Relay pulse at 0x%02X\n", address);
  } else {
    Serial.printf("[FAIL] Could not send pulse to 0x%02X\n", address);
  }
}

void setToggleTime(String cmd) {
  if (i2c_safe_mode) {
    Serial.println("[ERROR] I2C is in SAFE MODE - command unavailable");
    return;
  }

  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);
  if (first_space < 0 || second_space < 0) {
    Serial.println("Usage: pulse XX BB  (BB hex 01..28, time = BB * 25 ms)");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String block_str = cmd.substring(second_space + 1);
  addr_str.trim();
  block_str.trim();

  uint8_t address = parseHex(addr_str);
  uint8_t units = 0;
  if (address == 0 || !parseHexByte(block_str, &units) || units < 0x01 || units > 0x28) {
    Serial.println("ERROR: use pulse XX BB with BB hex 01..28");
    return;
  }

  if (!sendCommand(address, CMD_SET_TOGGLE_TIME)) {
    Serial.printf("[FAIL] Could not start configuration at 0x%02X\n", address);
    return;
  }

  delay(10);
  if (sendCommand(address, units)) {
    Serial.printf("[OK] Relay pulse at 0x%02X set to 0x%02X (%u ms)\n", address, units, units * 25);
  } else {
    Serial.printf("[FAIL] Could not store pulse time at 0x%02X\n", address);
  }
}

void getToggleTime(String cmd) {
  if (i2c_safe_mode) {
    Serial.println("[ERROR] I2C is in SAFE MODE - command unavailable");
    return;
  }

  String addr_str = cmd.substring(8);
  addr_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  uint8_t error = transmitCommandByte(address, CMD_GET_TOGGLE_TIME);
  if (error != 0) {
    Serial.printf("[FAIL] Could not read pulse time from 0x%02X\n", address);
    return;
  }

  uint8_t units = readResponse(address);
  if (units >= 1 && units <= 0x28) {
    Serial.printf("[OK] Relay pulse at 0x%02X = 0x%02X (%u ms)\n", address, units, units * 25);
  } else {
    Serial.printf("[FAIL] Invalid response 0x%02X from 0x%02X\n", units, address);
  }
}

bool isValidAdcAveraging(uint8_t sampleCount) {
  return sampleCount == 4 || sampleCount == 8 || sampleCount == 16 || sampleCount == 24;
}

void setAdcAveraging(String commandLine) {
  if (i2c_safe_mode) {
    Serial.println("[ERROR] I2C is in safe mode");
    return;
  }

  int firstSpace = commandLine.indexOf(' ');
  int secondSpace = commandLine.indexOf(' ', firstSpace + 1);
  if (firstSpace < 0 || secondSpace < 0) {
    Serial.println("Usage: average <address_hex> <4|8|16|24>");
    return;
  }

  String addressText = commandLine.substring(firstSpace + 1, secondSpace);
  String sampleCountText = commandLine.substring(secondSpace + 1);
  addressText.trim();
  sampleCountText.trim();

  uint8_t address = parseHex(addressText);
  char* end = nullptr;
  long parsedCount = strtol(sampleCountText.c_str(), &end, 10);
  if (address == 0 || end == sampleCountText.c_str() || *end != '\0' || parsedCount < 0 || parsedCount > 255 || !isValidAdcAveraging((uint8_t)parsedCount)) {
    Serial.println("[ERROR] Use: average <address_hex> <4|8|16|24>");
    return;
  }

  uint8_t sampleCount = (uint8_t)parsedCount;
  if (!sendCommand(address, CMD_SET_ADC_AVERAGING)) {
    Serial.printf("[FAIL] Could not start ADC averaging configuration on 0x%02X\n", address);
    return;
  }

  if (sendCommand(address, sampleCount)) {
    Serial.printf("[OK] ADC averaging on 0x%02X set to %u samples\n", address, sampleCount);
  } else {
    Serial.printf("[FAIL] Could not save ADC averaging on 0x%02X\n", address);
  }
}

void getAdcAveraging(String commandLine) {
  if (i2c_safe_mode) {
    Serial.println("[ERROR] I2C is in safe mode");
    return;
  }

  int firstSpace = commandLine.indexOf(' ');
  if (firstSpace < 0) {
    Serial.println("Usage: getaverage <address_hex>");
    return;
  }

  String addressText = commandLine.substring(firstSpace + 1);
  addressText.trim();
  uint8_t address = parseHex(addressText);
  if (address == 0) {
    Serial.println("[ERROR] Invalid I2C address");
    return;
  }

  uint8_t error = transmitCommandByte(address, CMD_GET_ADC_AVERAGING);
  if (error != 0) {
    Serial.printf("[FAIL] Could not read ADC averaging from 0x%02X\n", address);
    return;
  }

  uint8_t sampleCount = readResponse(address);
  if (isValidAdcAveraging(sampleCount)) {
    Serial.printf("[OK] ADC averaging on 0x%02X = %u samples\n", address, sampleCount);
  } else {
    Serial.printf("[FAIL] Invalid response 0x%02X from 0x%02X\n", sampleCount, address);
  }
}

void relayOn(String cmd) {
  String addr_str = cmd.substring(3);
  addr_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  if (sendCommand(address, CMD_RELAY_ON)) {
    Serial.printf("[OK] Relay ON at 0x%02X\n", address);
  } else {
    Serial.printf("[FAIL] Could not turn relay on at 0x%02X\n", address);
  }
}

void relayOff(String cmd) {
  String addr_str = cmd.substring(4);
  addr_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  if (sendCommand(address, CMD_RELAY_OFF)) {
    Serial.printf("[OK] Relay OFF at 0x%02X\n", address);
  } else {
    Serial.printf("[FAIL] Could not turn relay off at 0x%02X\n", address);
  }
}

// ═══════════════════════════════════════════════════════════
//           DIGITAL INPUT READ
// ═══════════════════════════════════════════════════════════

void readDigitalInputCommand(String cmd) {
  String addr_str = cmd.substring(5);
  addr_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  uint8_t state = 0;
  bool success = readDigitalInputState(address, &state);

  if (success) {
    Serial.printf("[OK] 0x%02X - PA%u digital: %s\n",
                  address, last_digital_input_pin, state ? "HIGH" : "LOW");
  } else {
    Serial.printf("[FAIL] Could not read digital input at 0x%02X\n", address);
  }
}

void startAutoRead(String cmd) {
  if (i2c_safe_mode) {
    Serial.println("[ERROR] I2C is in SAFE MODE - command unavailable");
    return;
  }

  cmd.trim();
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  String addr_str;
  String interval_str;
  if (second_space == -1) {
    addr_str = cmd.substring(first_space + 1);
  } else {
    addr_str = cmd.substring(first_space + 1, second_space);
    interval_str = cmd.substring(second_space + 1);
  }
  addr_str.trim();
  interval_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  uint32_t interval = 2000;
  if (interval_str.length() > 0) {
    long v = interval_str.toInt();
    if (v < 100) v = 100;
    if (v > 60000) v = 60000;
    interval = (uint32_t)v;
  }

  autoread_address = address;
  autoread_interval = interval;
  autoread_active = true;
  last_autoread_time = millis();

  Serial.printf("\n[OK] Autoread ENABLED at 0x%02X\n", address);
  Serial.printf("     Interval: %lu ms\n", autoread_interval);
  Serial.println("     Use 'stopautoread' or 'stop' to stop\n");
}

void stopAutoRead() {
  if (autoread_active) {
    autoread_active = false;
    Serial.println("\n[OK] Autoread STOPPED\n");
  }
}

void autoReadTick() {
  if (i2c_safe_mode) {
    stopAutoRead();
    Serial.println("[ERROR] I2C is in SAFE MODE - autoread stopped");
    return;
  }

  uint8_t state = 0;
  bool success = readDigitalInputState(autoread_address, &state);

  if (success) {
    Serial.printf("[OK] 0x%02X - PA%u digital: %s\n",
                  autoread_address, last_digital_input_pin, state ? "HIGH" : "LOW");
  } else {
    Serial.printf("[FAIL] Could not read digital input at 0x%02X\n", autoread_address);
  }
}

void readPA0DigitalInputCommand(String cmd) {
  String addr_str = cmd.substring(6);
  addr_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  uint8_t error = transmitCommandByte(address, CMD_PA0_DIGITAL);
  if (error != 0) {
    Serial.printf("[FAIL] Could not request digital PA0 at 0x%02X\n", address);
    return;
  }

  delay(5);
  WIRE.setTimeout(50);
  uint8_t bytes_read = WIRE.requestFrom(address, (uint8_t)1);
  if (bytes_read != 1) {
    while (WIRE.available()) {
      WIRE.read();
    }
    Serial.printf("[FAIL] Digital PA0 is not supported at 0x%02X\n", address);
    return;
  }

  uint8_t response = WIRE.read();
  if (response > 1) {
    Serial.printf("[FAIL] Digital PA0 is not supported at 0x%02X (response 0x%02X)\n",
                  address, response);
    return;
  }

  uint8_t state = response;
  Serial.printf("[OK] 0x%02X - PA0 digital: %s\n",
                address, state ? "HIGH" : "LOW");
}

bool readDigitalInputState(uint8_t address, uint8_t* state) {
  // i2c_slave joystick: PA0 responde directamente 0/1.
  uint8_t error = transmitCommandByte(address, CMD_PA0_DIGITAL);
  if (error != 0) {
    delay(5);
    error = transmitCommandByte(address, CMD_PA0_DIGITAL);
  }

  if (error == 0) {
    delay(5);
    WIRE.setTimeout(50);
    uint8_t bytes_read = WIRE.requestFrom(address, (uint8_t)1);
    if (bytes_read == 1) {
      uint8_t response = WIRE.read();
      if (response <= 1) {
        *state = response;
        last_digital_input_pin = 0;
        updateDigitalInputStats(address, true, *state);
        return true;
      }
    }
    while (WIRE.available()) {
      WIRE.read();
    }
  }

  // firmware does not implement PA0; use its legacy PA4 digital input.
  error = transmitCommandByte(address, CMD_PA4_DIGITAL);
  if (error != 0) {
    delay(5);
    error = transmitCommandByte(address, CMD_PA4_DIGITAL);
  }
  if (error != 0) {
    updateDigitalInputStats(address, false, 0);
    return false;
  }

  delay(5);
  WIRE.setTimeout(50);
  uint8_t bytesRead = WIRE.requestFrom(address, (uint8_t)1);

  if (bytesRead == 1) {
    uint8_t response = WIRE.read();
    *state = (response & 0xF0) ? 1 : 0;
    last_digital_input_pin = 4;
    updateDigitalInputStats(address, true, *state);
    return true;
  }

  updateDigitalInputStats(address, false, 0);
  return false;
}

// ═══════════════════════════════════════════════════════════
//           12-BIT ADC0 / ADC1 READ
// ═══════════════════════════════════════════════════════════

bool readADC12bitPacked(uint8_t address, uint8_t command, uint16_t* adc_value) {
  uint8_t error = transmitCommandByte(address, command);
  if (error != 0) {
    updateDeviceError(address, false);
    return false;
  }

  delay(10);
  WIRE.setTimeout(100);
  uint8_t bytes_read = WIRE.requestFrom(address, (uint8_t)2);
  if (bytes_read != 2) {
    while (WIRE.available()) {
      WIRE.read();
    }
    updateDeviceError(address, false);
    return false;
  }

  uint8_t hsb = WIRE.read();
  uint8_t lsb = WIRE.read();
  *adc_value = ((uint16_t)(hsb & 0x0F) << 8) | lsb;
  updateDeviceError(address, true);
  return true;
}

void readADCCommand(String cmd, bool adc1) {
  if (i2c_safe_mode) {
    Serial.println("[ERROR] I2C is in SAFE MODE - command unavailable");
    return;
  }

  String addr_str = cmd.substring(5);
  addr_str.trim();
  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  uint16_t adc_value = 0;
  uint8_t adc_command = adc1 ? CMD_ADC_PA1_12BIT : CMD_ADC_PA0_12BIT;
  if (readADC12bitPacked(address, adc_command, &adc_value)) {
    // Do not print a physical pin: ADC0/ADC1 remain protocol channel
    // protocol; its assignment depends on the firmware loaded on the node.
    Serial.printf("[OK] 0x%02X - ADC%d: %u (0x%03X)\n",
                  address, adc1 ? 1 : 0, adc_value, adc_value);
  } else {
    Serial.printf("[FAIL] Could not read ADC%d at 0x%02X\n",
                  adc1 ? 1 : 0, address);
  }
}

void updateDigitalInputStats(uint8_t address, bool success, uint8_t state) {
  // Find the device in the array
  int index = -1;
  for (int i = 0; i < digital_stats_count; i++) {
    if (digital_stats[i].address == address) {
      index = i;
      break;
    }
  }

  // Add it if it does not exist
  if (index == -1 && digital_stats_count < 128) {
    index = digital_stats_count++;
    digital_stats[index].address = address;
    digital_stats[index].read_count = 0;
    digital_stats[index].fail_count = 0;
    digital_stats[index].last_state = 0;
    digital_stats[index].last_read_time = 0;
  }

  if (index >= 0) {
    digital_stats[index].read_count++;
    if (!success) {
      digital_stats[index].fail_count++;
    } else {
      digital_stats[index].last_state = state;
    }
    digital_stats[index].last_read_time = millis();
  }
}

void showDigitalInputStats() {
  if (digital_stats_count == 0) {
    Serial.println("No digital-read statistics are available");
    return;
  }

  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║      DIGITAL INPUT STATISTICS                 ║");
  Serial.println("╠════════════════════════════════════════════════════════╣");
  Serial.println("║ Addr │ Reads │ Failures │ Success rate │ State │  Time ║");
  Serial.println("╠════════════════════════════════════════════════════════╣");

  for (int i = 0; i < digital_stats_count; i++) {
    DigitalInputStats* stats = &digital_stats[i];

    float success_rate = 0.0;
    if (stats->read_count > 0) {
      success_rate = ((float)(stats->read_count - stats->fail_count) / stats->read_count) * 100.0;
    }

    uint32_t time_since = (millis() - stats->last_read_time) / 1000;  // seconds

    Serial.printf("║ 0x%02X │ %8lu │ %6lu │ %6.1f%% │ %4s   │ %4lus   ║\n",
                  stats->address,
                  stats->read_count,
                  stats->fail_count,
                  success_rate,
                  stats->last_state ? "HIGH" : "LOW",
                  time_since);
  }

  Serial.println("╚════════════════════════════════════════════════════════╝");
}

// ═══════════════════════════════════════════════════════════
//           CONTINUOUS DIGITAL-READ TEST
// ═══════════════════════════════════════════════════════════

void startReadTest(String cmd) {
  // Format: "readtest <interval_ms> <duration_ms>"
  // Example: "readtest 100 10000" = read every 100 ms for 10 seconds
  cmd.trim();
  int space1 = cmd.indexOf(' ', 9);

  if (space1 == -1) {
    Serial.println("ERROR: Invalid format");
    Serial.println("Usage: readtest <interval_ms> <duration_ms>");
    Serial.println("Example: readtest 100 10000  (read every 100 ms for 10 seconds)");
    return;
  }

  String interval_str = cmd.substring(9, space1);
  String duration_str = cmd.substring(space1 + 1);

  read_test_interval = interval_str.toInt();
  read_test_duration = duration_str.toInt();

  // Validar rango
  if (read_test_interval < 30) read_test_interval = 30;
  if (read_test_interval > 1000) read_test_interval = 1000;
  if (read_test_duration < 1000) read_test_duration = 1000;

  // Reset digital-read counters
  digital_stats_count = 0;
  memset(digital_stats, 0, sizeof(digital_stats));

  read_test_active = true;
  read_test_start_time = millis();
  last_scan_time = millis();

  Serial.println("\n[READ TEST MODE ENABLED]");
  Serial.printf("Interval: %lu ms\n", read_test_interval);
  Serial.printf("Duration: %lu ms (%.1f s)\n", read_test_duration, read_test_duration / 1000.0);
  Serial.println("Devices to read: ");
  for (int i = 0; i < device_count; i++) {
    Serial.printf("  0x%02X\n", found_devices[i]);
  }
  Serial.println("Use 'stopread' to stop early\n");
}

void stopReadTest() {
  read_test_active = false;
  uint32_t elapsed = millis() - read_test_start_time;
  Serial.println("\n[READ TEST MODE STOPPED]");
  Serial.printf("Elapsed time: %.2f seconds\n", elapsed / 1000.0);
  showDigitalInputStats();
}

void testDigitalInputReads() {
  WIRE.setTimeout(50);

  for (int i = 0; i < device_count; i++) {
    uint8_t addr = found_devices[i];
    uint8_t state = 0;
    readDigitalInputState(addr, &state);
    delay(read_test_interval / device_count);  // Distribute time between devices
  }
}

uint8_t transmitCommandByte(uint8_t address, uint8_t command) {
  if (i2c_command_sent) {
    uint32_t elapsed = millis() - last_i2c_command_time;
    if (elapsed < MIN_COMMAND_INTERVAL_MS) {
      delay(MIN_COMMAND_INTERVAL_MS - elapsed);
    }
  }

  WIRE.beginTransmission(address);
  WIRE.write(command);
  uint8_t error = WIRE.endTransmission();
  last_i2c_command_time = millis();
  i2c_command_sent = true;
  return error;
}

bool sendCommand(uint8_t address, uint8_t command) {
  uint8_t error = transmitCommandByte(address, command);

  if (error != 0) return false;

  // Wait for a response with a more generous timeout
  delay(20);  // Give the slave time to process
  WIRE.setTimeout(100);
  uint8_t bytesRead = WIRE.requestFrom(address, (uint8_t)1);
  if (bytesRead == 1) {
    WIRE.read();  // Read and discard the response
    return true;
  }

  return false;
}

// Function that reads a device response
uint8_t readResponse(uint8_t address) {
  delay(10);  // Short delay that lets the device prepare its response
  WIRE.setTimeout(100);
  uint8_t bytesRead = WIRE.requestFrom(address, (uint8_t)1);
  if (bytesRead == 1) {
    return WIRE.read();
  }
  return 0xFF;  // Error: no response was received
}

// Fast path: send a command and consume the slave response.
// If this byte is not read, the PY32 remains in HAL_I2C_Slave_Transmit()
// until its 100 ms timeout expires, which can make the next cycle fail.
bool sendCommandFast(uint8_t address, uint8_t command) {
  uint8_t error = transmitCommandByte(address, command);

  if (error != 0) {
    updateDeviceError(address, false);
    return false;
  }

  // The PY32 RX callback only marks the command as pending. Allow a
  // short interval for the main loop to prepare the response byte.
  delay(2);
  WIRE.setTimeout(20);
  uint8_t bytes_read = WIRE.requestFrom(address, (uint8_t)1);
  bool success = (bytes_read == 1);

  if (success) {
    WIRE.read();  // Consume the ACK and release the slave immediately.
  } else {
    while (WIRE.available()) {
      WIRE.read();
    }
  }

  updateDeviceError(address, success);
  return success;
}

bool sendByteNoRead(uint8_t address, uint8_t value) {
  uint8_t error = transmitCommandByte(address, value);
  return (error == 0);
}

bool writeRegisterValue(uint8_t address, uint8_t reg, uint8_t value) {
  // The RGB slave processes write-only data byte by byte: register, then value.
  bool ok = sendByteNoRead(address, reg);
  if (!ok) {
    delay(3);
    ok = sendByteNoRead(address, reg);
  }

  if (!ok) {
    updateDeviceError(address, false);
    return false;
  }

  delay(4);

  ok = sendByteNoRead(address, value);
  if (!ok) {
    delay(3);
    ok = sendByteNoRead(address, value);
  }

  updateDeviceError(address, ok);
  return ok;
}

bool parseHexByte(String hex_str, uint8_t* value) {
  hex_str.trim();
  char* endptr;
  long v = strtol(hex_str.c_str(), &endptr, 16);

  if (*endptr != '\0' || v < 0 || v > 0xFF) {
    return false;
  }

  *value = (uint8_t)v;
  return true;
}

void writeRegisterCommand(String cmd) {
  // Format: wr <address> <register> <value>
  cmd.trim();
  int space1 = cmd.indexOf(' ', 3);
  int space2 = cmd.indexOf(' ', space1 + 1);

  if (space1 == -1 || space2 == -1) {
    Serial.println("ERROR: Invalid format");
    Serial.println("Usage: wr <addr> <reg> <val>");
    Serial.println("Example: wr 20 60 ff");
    return;
  }

  String addr_str = cmd.substring(3, space1);
  String reg_str = cmd.substring(space1 + 1, space2);
  String value_str = cmd.substring(space2 + 1);
  addr_str.trim();
  reg_str.trim();
  value_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address (0x08-0x77)");
    return;
  }

  uint8_t reg = 0;
  uint8_t value = 0;
  if (!parseHexByte(reg_str, &reg) || !parseHexByte(value_str, &value)) {
    Serial.println("ERROR: Invalid register/value (00-FF)");
    return;
  }

  if (writeRegisterValue(address, reg, value)) {
    Serial.printf("[OK] 0x%02X <- [0x%02X] = 0x%02X\n", address, reg, value);
  } else {
    Serial.printf("[FAIL] Could not write 0x%02X to [0x%02X] at 0x%02X\n", value, reg, address);
  }
}

void rgbRegisterCommand(String cmd) {
  // Format: rgb <address> <red> <green> <blue>
  cmd.trim();
  int s1 = cmd.indexOf(' ', 4);
  int s2 = cmd.indexOf(' ', s1 + 1);
  int s3 = cmd.indexOf(' ', s2 + 1);

  if (s1 == -1 || s2 == -1 || s3 == -1) {
    Serial.println("ERROR: Invalid format");
    Serial.println("Usage: rgb <addr> <red> <green> <blue>");
    Serial.println("Example: rgb 20 ff 00 40");
    return;
  }

  String addr_str = cmd.substring(4, s1);
  String red_str = cmd.substring(s1 + 1, s2);
  String green_str = cmd.substring(s2 + 1, s3);
  String blue_str = cmd.substring(s3 + 1);
  addr_str.trim();
  red_str.trim();
  green_str.trim();
  blue_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address (0x08-0x77)");
    return;
  }

  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
  if (!parseHexByte(red_str, &red) || !parseHexByte(green_str, &green) || !parseHexByte(blue_str, &blue)) {
    Serial.println("ERROR: Invalid RGB value (00-FF)");
    return;
  }

  bool ok_red = writeRegisterValue(address, REG_RGB_RED, red);
  delay(2);
  bool ok_green = writeRegisterValue(address, REG_RGB_GREEN, green);
  delay(2);
  bool ok_blue = writeRegisterValue(address, REG_RGB_BLUE, blue);

  if (ok_red && ok_green && ok_blue) {
    Serial.printf("[OK] RGB 0x%02X -> R:0x%02X G:0x%02X B:0x%02X\n", address, red, green, blue);
  } else {
    Serial.printf("[FAIL] Partial/failed RGB write at 0x%02X (R:%s G:%s B:%s)\n",
                  address,
                  ok_red ? "OK" : "FAIL",
                  ok_green ? "OK" : "FAIL",
                  ok_blue ? "OK" : "FAIL");
  }
}

void rgbOffRegisterCommand(String cmd) {
  // Format: offrgb <address>
  String addr_str = cmd.substring(7);
  addr_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address (0x08-0x77)");
    return;
  }

  bool ok = true;
  ok = ok && writeRegisterValue(address, REG_RGB_RED, 0x00);
  ok = ok && writeRegisterValue(address, REG_RGB_GREEN, 0x00);
  ok = ok && writeRegisterValue(address, REG_RGB_BLUE, 0x00);
  ok = ok && sendCommand(address, CMD_OFF);

  if (ok) {
    Serial.printf("[OK] RGB OFF at 0x%02X (RGB=0 + CMD_OFF)\n", address);
  } else {
    Serial.printf("[FAIL] Could not turn RGB off at 0x%02X\n", address);
  }
}

void redSweepCommand(String cmd) {
  // Format: sweepr <address> [delay_ms]
  cmd.trim();
  int space = cmd.indexOf(' ', 7);

  String addr_str;
  String delay_str;
  if (space == -1) {
    addr_str = cmd.substring(7);
  } else {
    addr_str = cmd.substring(7, space);
    delay_str = cmd.substring(space + 1);
  }

  addr_str.trim();
  delay_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address (0x08-0x77)");
    return;
  }

  uint16_t step_delay = 8;
  if (delay_str.length() > 0) {
    long d = delay_str.toInt();
    if (d < 1) d = 1;
    if (d > 1000) d = 1000;
    step_delay = (uint16_t)d;
  }

  if (!writeRegisterValue(address, REG_RGB_GREEN, 0x00) || !writeRegisterValue(address, REG_RGB_BLUE, 0x00)) {
    Serial.printf("[FAIL] Could not prepare red sweep at 0x%02X\n", address);
    return;
  }

  Serial.printf("[INFO] Red sweep at 0x%02X (delay %u ms)\n", address, step_delay);
  for (uint16_t value = 0; value <= 0xFF; value++) {
    if (!writeRegisterValue(address, REG_RGB_RED, (uint8_t)value)) {
      Serial.printf("[FAIL] Red sweep error at value 0x%02X\n", (uint8_t)value);
      return;
    }
    delay(step_delay);
  }

  Serial.printf("[OK] Red sweep completed at 0x%02X\n", address);
}

void updateDeviceError(uint8_t address, bool success) {
  // Find the device in the array
  int index = -1;
  for (int i = 0; i < device_error_count; i++) {
    if (device_errors[i].address == address) {
      index = i;
      break;
    }
  }

  // Add it if it does not exist
  if (index == -1 && device_error_count < 128) {
    index = device_error_count++;
    device_errors[index].address = address;
    device_errors[index].error_count = 0;
    device_errors[index].success_count = 0;
    device_errors[index].last_error_time = 0;
  }

  // Actualizar contadores
  if (index != -1) {
    if (success) {
      device_errors[index].success_count++;
    } else {
      device_errors[index].error_count++;
      device_errors[index].last_error_time = millis();
    }
  }
}

void startTest(String cmd) {
  // Format: "test <interval_ms> <duration_ms>"
  // Example: "test 50 5000" = test every 50 ms for 5 seconds
  cmd.trim();
  int space1 = cmd.indexOf(' ', 5);

  if (space1 == -1) {
    Serial.println("ERROR: Invalid format");
    Serial.println("Usage: test <interval_ms> <duration_ms>");
    Serial.println("Example: test 50 5000  (test every 50 ms for 5 seconds)");
    return;
  }

  String interval_str = cmd.substring(5, space1);
  String duration_str = cmd.substring(space1 + 1);

  test_interval = interval_str.toInt();
  test_duration = duration_str.toInt();

  // Validar rango
  if (test_interval < 30) test_interval = 30;
  if (test_interval > 1000) test_interval = 1000;
  if (test_duration < 1000) test_duration = 1000;

  // Reset contadores
  device_error_count = 0;
  memset(device_errors, 0, sizeof(device_errors));

  test_mode_active = true;
  test_start_time = millis();
  last_scan_time = millis();

  Serial.println("\n[TEST MODE ENABLED]");
  Serial.printf("Interval: %lu ms\n", test_interval);
  Serial.printf("Duration: %lu ms (%.1f s)\n", test_duration, test_duration / 1000.0);
  Serial.println("Use 'stoptest' to stop early\n");
}

void stopTest() {
  test_mode_active = false;
  uint32_t elapsed = millis() - test_start_time;
  Serial.println("\n[TEST MODE STOPPED]");
  Serial.printf("Elapsed time: %.2f seconds\n\n", elapsed / 1000.0);
}

void testDevices() {
  // Check whether safe mode is active
  if (i2c_safe_mode) {
    Serial.println("[ERROR] I2C is in SAFE MODE - command unavailable");
    return;
  }

  WIRE.setTimeout(50);

  for (int i = 0; i < device_count; i++) {
    uint8_t addr = found_devices[i];
    sendCommandFast(addr, CMD_RELAY_TOGGLE);
    delay(test_interval / device_count);  // Distribute time between devices
  }
}

void showTestResults() {
  Serial.println("\n╔═══════════════════════════════════════════════════╗");
  Serial.println("║           I2C TEST RESULTS                 ║");
  Serial.println("╚═══════════════════════════════════════════════════╝\n");

  if (device_error_count == 0) {
    Serial.println("No test data available\n");
    return;
  }

  uint32_t total_success = 0;
  uint32_t total_errors = 0;

  Serial.println("Addr  | Success | Errors | Success rate | Last error");
  Serial.println("------|---------|--------|------------|-------------");

  for (int i = 0; i < device_error_count; i++) {
    uint8_t addr = device_errors[i].address;
    uint32_t success = device_errors[i].success_count;
    uint32_t errors = device_errors[i].error_count;
    uint32_t total = success + errors;
    float rate = total > 0 ? (success * 100.0 / total) : 0;

    total_success += success;
    total_errors += errors;

    Serial.printf("0x%02X  | %7lu | %6lu | %6.2f%%   | ",
                  addr, success, errors, rate);

    if (errors > 0) {
      uint32_t since_error = (millis() - device_errors[i].last_error_time) / 1000;
      Serial.printf("%lu s\n", since_error);
    } else {
      Serial.println("Nunca");
    }
  }

  Serial.println("------|---------|--------|------------|-------------");
  uint32_t grand_total = total_success + total_errors;
  float overall_rate = grand_total > 0 ? (total_success * 100.0 / grand_total) : 0;
  Serial.printf("TOTAL | %7lu | %6lu | %6.2f%%   |\n\n",
                total_success, total_errors, overall_rate);
}

uint8_t parseHex(String hex_str) {
  hex_str.trim();

  // Convert from hexadecimal
  char* endptr;
  long addr = strtol(hex_str.c_str(), &endptr, 16);

  if (*endptr != '\0' || addr < 0x08 || addr > 0x77) {
    return 0;
  }

  return (uint8_t)addr;
}

bool parseByteAuto(String value_str, uint8_t* out) {
  value_str.trim();
  char* endptr;
  long value = strtol(value_str.c_str(), &endptr, 0);

  if (*endptr != '\0' || value < 0 || value > 0xFF) {
    return false;
  }

  *out = (uint8_t)value;
  return true;
}

void startScanLoop() {
  scan_loop_active = true;
  last_scan_time = millis();
  Serial.println("\n[OK] Continuous scan ENABLED");
  Serial.printf("     Interval: %lu ms\n", scan_interval);
  Serial.println("     Use 'stop' to stop\n");
}

void stopScanLoop() {
  scan_loop_active = false;
  Serial.println("\n[OK] Continuous scan STOPPED\n");
}

// ═══════════════════════════════════════════════════════════
//           FUNCIONES PWM / BUZZER
// ═══════════════════════════════════════════════════════════

void pwmOff() {
  Serial.println("\n━━━ TURN PWM OFF (ALL DEVICES) ━━━");
  int count = 0;
  for (int i = 0; i < device_count; i++) {
    uint8_t addr = found_devices[i];
    if (sendCommand(addr, CMD_PWM_OFF)) {
      Serial.printf("  [OK] 0x%02X - PWM apagado\n", addr);
      count++;
    } else {
      Serial.printf("  [FAIL] 0x%02X\n", addr);
    }
    delay(10);
  }
  Serial.printf("\nTotal: %d/%d devices\n", count, device_count);
}

void pwmCommand(String cmd) {
  // Format: "pwm XX LEVEL"
  // XX = hex address, LEVEL = 25, 50, 75, 100, off
  cmd.trim();
  int space = cmd.indexOf(' ', 4);

  if (space == -1) {
    Serial.println("ERROR: Invalid format");
    Serial.println("Usage: pwm XX LEVEL");
    Serial.println("Example: pwm 20 50  (PWM 50% at 0x20)");
    Serial.println("Niveles: 25, 50, 75, 100, off");
    return;
  }

  String addr_str = cmd.substring(4, space);
  String level_str = cmd.substring(space + 1);
  addr_str.trim();
  level_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  uint8_t pwm_cmd = CMD_PWM_OFF;
  String level_name = "OFF";

  if (level_str == "off" || level_str == "0") {
    pwm_cmd = CMD_PWM_OFF;
    level_name = "OFF";
  } else if (level_str == "25") {
    pwm_cmd = CMD_PWM_25;
    level_name = "25% (200Hz)";
  } else if (level_str == "50") {
    pwm_cmd = CMD_PWM_50;
    level_name = "50% (500Hz)";
  } else if (level_str == "75") {
    pwm_cmd = CMD_PWM_75;
    level_name = "75% (1000Hz)";
  } else if (level_str == "100") {
    pwm_cmd = CMD_PWM_100;
    level_name = "100% (2000Hz)";
  } else {
    Serial.println("ERROR: Invalid level. Use: 25, 50, 75, 100, off");
    return;
  }

  if (sendCommand(address, pwm_cmd)) {
    Serial.printf("[OK] PWM %s at 0x%02X\n", level_name.c_str(), address);
  } else {
    Serial.printf("[FAIL] Could not configure PWM at 0x%02X\n", address);
  }
}

// ═══════════════════════════════════════════════════════════
//           FUNCIONES SLOT LED RGB
// ═══════════════════════════════════════════════════════════

bool resolveSlotCommand(String slot_str, uint8_t* command, uint8_t* slot) {
  slot_str.trim();
  slot_str.toLowerCase();

  if (slot_str == "1" || slot_str == "red") {
    *command = CMD_RED;
    *slot = 1;
    return true;
  }
  if (slot_str == "2" || slot_str == "green") {
    *command = CMD_GREEN;
    *slot = 2;
    return true;
  }
  if (slot_str == "3" || slot_str == "blue") {
    *command = CMD_BLUE;
    *slot = 3;
    return true;
  }

  return false;
}

void sendSlotCommand(uint8_t address, uint8_t slot, uint8_t command) {
  if (sendCommand(address, command)) {
    Serial.printf("[OK] Slot %u applied at 0x%02X\n", slot, address);
  } else {
    Serial.printf("[FAIL] Could not apply slot %u at 0x%02X\n", slot, address);
  }
}

void slotCommand(String cmd) {
  // Format: "slot XX N" (N=1..3). "led/neo" remain legacy aliases.
  cmd.trim();
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Invalid format");
    Serial.println("Usage: slot XX N");
    Serial.println("Example: slot 20 1  (apply slot 1 at 0x20)");
    Serial.println("Slots: 1, 2, 3");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String slot_str = cmd.substring(second_space + 1);
  addr_str.trim();
  slot_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  uint8_t slot_command = 0;
  uint8_t slot = 0;
  if (!resolveSlotCommand(slot_str, &slot_command, &slot)) {
    Serial.println("ERROR: Invalid slot. Use: 1, 2, or 3");
    return;
  }

  sendSlotCommand(address, slot, slot_command);
}

void slotShortcutCommand(String cmd, uint8_t slot) {
  String addr_str = cmd.substring(cmd.indexOf(' ') + 1);
  addr_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  uint8_t slot_command = 0;
  uint8_t parsed_slot = 0;
  if (!resolveSlotCommand(String(slot), &slot_command, &parsed_slot)) {
    Serial.println("ERROR: Invalid slot. Use: 1, 2, or 3");
    return;
  }

  sendSlotCommand(address, parsed_slot, slot_command);
}

void neoWhite(String cmd) {
  String addr_str = cmd.substring(6);
  addr_str.trim();

  uint8_t address = parseHex(addr_str);
  if (address == 0) {
    Serial.println("ERROR: Invalid address");
    return;
  }

  if (sendCommand(address, CMD_WHITE)) {
    Serial.printf("[OK] Direct RGB LED white at 0x%02X\n", address);
  } else {
    Serial.printf("[FAIL] Could not apply direct white at 0x%02X\n", address);
  }
}

void neoOff() {
  Serial.println("\n━━━ TURN RGB LED OFF (ALL DEVICES) ━━━");
  int count = 0;
  for (int i = 0; i < device_count; i++) {
    uint8_t addr = found_devices[i];
    if (sendCommand(addr, CMD_OFF)) {
      Serial.printf("  [OK] 0x%02X - LED RGB apagado\n", addr);
      count++;
    } else {
      Serial.printf("  [FAIL] 0x%02X\n", addr);
    }
    delay(10);
  }
  Serial.printf("\nTotal: %d/%d devices\n", count, device_count);
}

// ═══════════════════════════════════════════════════════════
//           FUNCTION: CHANGE I2C ADDRESS
// ═══════════════════════════════════════════════════════════
void changeI2CAddress(String cmd) {
  // Format: "ch XX YY", where XX=current address and YY=new address
  cmd.trim();
  int firstSpace = cmd.indexOf(' ');
  int secondSpace = cmd.indexOf(' ', firstSpace + 1);

  if (firstSpace == -1 || secondSpace == -1) {
    Serial.println("[ERROR] Format: ch XX YY");
    Serial.println("        XX = current address (hex)");
    Serial.println("        YY = new address (hex)");
    return;
  }

  String oldAddrStr = cmd.substring(firstSpace + 1, secondSpace);
  String newAddrStr = cmd.substring(secondSpace + 1);

  oldAddrStr.trim();
  newAddrStr.trim();

  // Convert addresses from hexadecimal
  uint8_t oldAddr = (uint8_t)strtol(oldAddrStr.c_str(), NULL, 16);
  uint8_t newAddr = (uint8_t)strtol(newAddrStr.c_str(), NULL, 16);

  // Validar rangos
  if (oldAddr < 0x08 || oldAddr > 0x77) {
    Serial.printf("[ERROR] Current address 0x%02X out of range (0x08-0x77)\n", oldAddr);
    return;
  }

  if (newAddr < 0x08 || newAddr > 0x77) {
    Serial.printf("[ERROR] New address 0x%02X out of range (0x08-0x77)\n", newAddr);
    return;
  }

  Serial.println("\n━━━ CHANGE I2C ADDRESS ━━━");
  Serial.printf("Device: 0x%02X → 0x%02X\n", oldAddr, newAddr);
  Serial.println("⚠️  WARNING: This operation is PERMANENT (stored in Flash)");
  Serial.println("");

  // CORRECT PROTOCOL according to firmware i2c_slave/src/main.c:
  // 1. Send 0x3D (CMD_SET_I2C_ADDR) - enables "wait for new address" mode
  // 2. The firmware responds with 0x0D and expects the next byte as the new address
  // 3. Send the new address as the next command
  // 4. Firmware stores the address in Flash and responds with 0x0D
  // 5. A reset is required to apply it

  Serial.print("Step 1: Enabling address-change mode... ");

  uint8_t error = transmitCommandByte(oldAddr, CMD_SET_I2C_ADDR);  // 0x3D

  if (error != 0) {
    Serial.printf("[FAIL] Error I2C: %d\n", error);
    Serial.println("⚠️  The device did not respond. Check:");
    Serial.println("   - Correct address (use 'scan' to confirm)");
    Serial.println("   - Device is connected and working");
    return;
  }

  delay(50);  // Important delay: firmware must process the command

  // Read response
  uint8_t bytesRead = WIRE.requestFrom(oldAddr, (uint8_t)1);
  uint8_t response = 0xFF;
  if (bytesRead == 1) {
    response = WIRE.read();
  }

  if (response == 0x0D) {  // RESP_I2C_ADDR_SET
    Serial.println("[OK] Mode enabled");
  } else {
    Serial.printf("[WARN] Response: 0x%02X (expected 0x0D)\n", response);
    // Continue; the operation may still work
  }

  delay(50);  // Additional delay before the next command

  // Step 2: Send the new address
  Serial.printf("Step 2: Sending new address 0x%02X... ", newAddr);

  error = transmitCommandByte(oldAddr, newAddr);  // New address as data

  if (error != 0) {
    Serial.printf("[FAIL] Error I2C: %d\n", error);
    Serial.println("⚠️  Could not complete the change");
    return;
  }

  delay(100);  // Long delay: the Flash write can take time

  // Read confirmation
  bytesRead = WIRE.requestFrom(oldAddr, (uint8_t)1);
  response = 0xFF;
  if (bytesRead == 1) {
    response = WIRE.read();
  }

  if (response == 0x0D) {  // RESP_I2C_ADDR_SET
    Serial.println("[OK] ✅ Stored in Flash");
  } else {
    Serial.printf("[WARN] Response: 0x%02X\n", response);
  }

  Serial.println("");
  Serial.println("╔════════════════════════════════════════════╗");
  Serial.println("║  ✅ ADDRESS CHANGE COMPLETED        ║");
  Serial.println("╚════════════════════════════════════════════╝");
  Serial.println("");
  Serial.println("⚠️  IMPORTANT - NEXT STEPS:");
  Serial.println("   1. The new address is STORED IN FLASH");
  Serial.println("   2. YOU MUST RESET the device to apply it");
  Serial.println("   3. Reset options:");
  Serial.println("      • Disconnect and reconnect the device");
  Serial.println("      • Use the physical reset button");
  Serial.println("      • Power-cycle the device");
  Serial.printf("   4. After reset: device at 0x%02X\n", newAddr);
  Serial.println("");
  Serial.println("Verification:");
  Serial.println("   1. Reset the device");
  Serial.println("   2. Ejecuta: scan");
  Serial.printf("   3. Find address 0x%02X in the list\n", newAddr);
  Serial.println("");
}

void saveSlotPreset(String cmd) {
  // Format: save_slot <addr> <slot> <r> <g> <b>
  cmd.trim();

  String tokens[6];
  int token_count = 0;
  int i = 0;
  while (i < cmd.length() && token_count < 6) {
    while (i < cmd.length() && cmd.charAt(i) == ' ') {
      i++;
    }
    if (i >= cmd.length()) {
      break;
    }

    int start = i;
    while (i < cmd.length() && cmd.charAt(i) != ' ') {
      i++;
    }
    tokens[token_count++] = cmd.substring(start, i);
  }

  if (token_count < 6 || (tokens[0] != "save_slot" && tokens[0] != "save_color")) {
    Serial.println("[ERROR] Format: save_slot <addr> <slot> <r> <g> <b>");
    Serial.println("        slot: 1, 2 o 3");
    Serial.println("        r/g/b: 0..255 (decimal) o 0x00..0xFF");
    return;
  }

  String addr_str = tokens[1];
  String pos_str = tokens[2];
  String r_str = tokens[3];
  String g_str = tokens[4];
  String b_str = tokens[5];

  addr_str.trim();
  pos_str.trim();
  r_str.trim();
  g_str.trim();
  b_str.trim();

  uint8_t addr = parseHex(addr_str);
  if (addr == 0) {
    Serial.println("[ERROR] Invalid address (0x08-0x77)");
    return;
  }

  uint8_t pos_user = 0;
  uint8_t pos = 0;
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if (!parseByteAuto(pos_str, &pos_user) || pos_user < 1 || pos_user > 3) {
    Serial.println("[ERROR] Invalid position. Use 1, 2, or 3");
    return;
  }
  pos = (uint8_t)(pos_user - 1);

  if (!parseByteAuto(r_str, &r) || !parseByteAuto(g_str, &g) || !parseByteAuto(b_str, &b)) {
    Serial.println("[ERROR] Invalid RGB value. Use 0..255 or 0x00..0xFF");
    return;
  }

  Serial.println("\n━━━ SAVE SLOT ━━━");
  Serial.printf("Device 0x%02X, slot %u, RGB=(%u,%u,%u)\n", addr, pos_user, r, g, b);

  if (!sendCommand(addr, CMD_SAVE_COLOR)) {
    Serial.println("[FAIL] Could not start save_slot");
    return;
  }

  delay(5);
  if (!sendCommand(addr, pos)) {
    Serial.println("[FAIL] Could not send position");
    return;
  }

  delay(5);
  if (!sendCommand(addr, r)) {
    Serial.println("[FAIL] Could not send channel R");
    return;
  }

  delay(5);
  if (!sendCommand(addr, g)) {
    Serial.println("[FAIL] Could not send channel G");
    return;
  }

  delay(5);
  if (!sendCommand(addr, b)) {
    Serial.println("[FAIL] Could not send channel B");
    return;
  }

  Serial.println("[OK] Slot stored in Flash");
  Serial.println("Use 'slot <addr> 1', 'slot <addr> 2', or 'slot <addr> 3' to test.");
}

void showHelp() {
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║        AVAILABLE COMMANDS          ║");
  Serial.println("╚═══════════════════════════════════════╝");
  Serial.println("");
  Serial.println("SCAN:");
  Serial.println("  s              - Scan I2C devices");
  Serial.println("  loop           - Continuous scan (1 s)");
  Serial.println("  stop           - Stop continuous scan");
  Serial.println("  recover        - Release and reinitialize the I2C bus");
  Serial.println("");
  Serial.println("TEST I2C:");
  Serial.println("  test INT DUR   - Asynchronous test");
  Serial.println("                   INT = interval (30-1000 ms)");
  Serial.println("                   DUR = duration (ms)");
  Serial.println("  stoptest       - Stop test and show results");
  Serial.println("  errors         - Show error statistics");
  Serial.println("");
  Serial.println("DIGITAL INPUT READ:");
  Serial.println("  read XX        - Read PA0; fall back to PA4 for ADC2 firmware");
  Serial.println("  read0 XX       - Read digital PA0 (i2c_slave joystick)");
  Serial.println("  dstats         - Digital input statistics");
  Serial.println("  readtest I D   - Continuous digital-input test");
  Serial.println("                   I = interval (30-1000 ms)");
  Serial.println("                   D = duration (ms)");
  Serial.println("  stopread       - Stop read test");
  Serial.println("");
  Serial.println("ADC:");
  Serial.println("  adc0 XX        - Read PA0/ADC_IN0 (0..4095)");
  Serial.println("  adc1 XX        - Read PA1/ADC_IN1 (0..4095)");
  Serial.println("  average XX N   - Set ADC averaging (N = 4, 8, 16 or 24)");
  Serial.println("  getaverage XX  - Read ADC averaging sample count");
  Serial.println("");
  Serial.println("RELAY:");
  Serial.println("  t XX           - Relay pulse");
  Serial.println("  pulse XX BB    - Change relay pulse time: hex block 01..28");
  Serial.println("                   Estructura: pulse <addr_hex> <bloque_hex>");
  Serial.println("                   Time = BB * 25 ms");
  Serial.println("  time XX BB     - Alias for pulse");
  Serial.println("  gettime XX     - Read relay pulse time");
  Serial.println("  on XX          - Turn relay on");
  Serial.println("  off XX         - Turn relay off");
  Serial.println("");
  Serial.println("PWM / BUZZER:");
  Serial.println("  pwm XX NIVEL   - PWM/buzzer (25, 50, 75, 100, off)");
  Serial.println("  silence        - Turn PWM off on all devices");
  Serial.println("");
  Serial.println("LED RGB:");
  Serial.println("  slot XX N      - Apply slot LED RGB (N=1..3)");
  Serial.println("  slot1 XX       - Apply slot 1");
  Serial.println("  slot2 XX       - Apply slot 2");
  Serial.println("  slot3 XX       - Apply slot 3");
  Serial.println("  white XX       - RGB LED white");
  Serial.println("  ledoff         - Turn RGB LED off on all devices");
  Serial.println("  led/neo/red/green/blue/neooff - Alias compatibles");
  Serial.println("");
  Serial.println("REGISTER-BASED RGB (PERSISTENT ON SLAVE):");
  Serial.println("  wr A R V       - Write register: <address> <register> <color>");
  Serial.println("  rgb A R G B    - Direct RGB through registers");
  Serial.println("                   R->0x60, B->0x62, G->0x64");
  Serial.println("  offrgb A       - Turn persistent color off (RGB=0 + CMD_OFF)");
  Serial.println("  save_slot A N R G B - Store RGB in a Flash slot");
  Serial.println("  sweepr A D     - Red sweep 0x00..0xFF (D ms opcional)");
  Serial.println("");
  Serial.println("I2C ADDRESS:");
  Serial.println("  ch XX YY       - Change I2C address");
  Serial.println("                   XX = current address (hex)");
  Serial.println("                   YY = new address (hex)");
  Serial.println("                   (stored in Flash; requires reset)");
  Serial.println("");
  Serial.println("FORMAT:");
  Serial.println("  XX = I2C address in hexadecimal");
  Serial.println("");
  Serial.println("EJEMPLOS:");
  Serial.println("  s              → Full scan");
  Serial.println("  loop           → Automatic scan every second");
  Serial.println("  stop           → Stop automatic scan");
  Serial.println("  test 50 10000  → Test every 50 ms for 10 s");
  Serial.println("  stoptest       → Stop test and show errors");
  Serial.println("  errors         → Show statistics");
  Serial.println("  read 20        → Read PA0 (or PA4 on ADC2 firmware)");
  Serial.println("  read0 20       → Read digital PA0 at 0x20");
  Serial.println("  readtest 50 10000 → Read test every 50 ms for 10 s");
  Serial.println("  stopread       → Stop read test");
  Serial.println("  dstats         → Show digital-input statistics");
  Serial.println("  adc0 20        → Read PA0/ADC_IN0 at 0x20");
  Serial.println("  adc1 20        → Read PA1/ADC_IN1 at 0x20");
  Serial.println("  average 20 16  → Average 16 ADC samples on 0x20");
  Serial.println("  getaverage 20  → Read ADC averaging on 0x20");
  Serial.println("  pwm 20 50      → PWM 50% (500 Hz) at 0x20");
  Serial.println("  silence        → Turn PWM off on all devices");
  Serial.println("  slot 20 1      → Apply slot 1 at 0x20");
  Serial.println("  slot1 20       → Apply slot 1 at 0x20");
  Serial.println("  ledoff         → Turn RGB LED off on all devices");
  Serial.println("  wr 20 60 ff    → Maximum red (registro 0x60)");
  Serial.println("  wr 20 62 40    → Medium blue (registro 0x62)");
  Serial.println("  wr 20 64 10    → Low green (registro 0x64)");
  Serial.println("  rgb 20 ff 10 00 → RGB through registers");
  Serial.println("  offrgb 20      → Persistent OFF");
  Serial.println("  save_slot 20 1 0x00 0xff 0x60 → Store RGB in slot 1");
  Serial.println("  sweepr 20 4    → Red sweep (4 ms per step)");
  Serial.println("  t 20           → Pulse at 0x20");
  Serial.println("  pulse 20 05    → Change relay pulse to 125ms");
  Serial.println("  pulse 20 28    → Change relay pulse to 1000ms");
  Serial.println("  gettime 20     → Read pulse time");
  Serial.println("  on 32          → Turn on at 0x32");
  Serial.println("  off 20         → Turn relay off at 0x20");
  Serial.println("  ch 19 25       → Change 0x19 to 0x25");
  Serial.println("");
}