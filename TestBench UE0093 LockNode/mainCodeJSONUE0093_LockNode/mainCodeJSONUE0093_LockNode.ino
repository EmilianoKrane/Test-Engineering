/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL UE0093 I2C LockNode JSON ====
*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <MapController.h>

// ==== Declaración de pines
#define RUN_BUTTON 4    // Botón de Arranque
#define RX2 D4          // GPIO15 como RX para PagWeb
#define TX2 D5          // GPIO19 como TX para PagWeb
#define I2C_SDA 6       // Comunicación con tarjeta LockNode por I2C
#define I2C_SCL 7       // Comunicación con tarjeta LockNode por I2C
#define RELAYNO 2       // Lectura de valores analógicos por conmutación de relé
#define RELAYNC 3       // Lectura de valores analógicos por conmutación de relé
#define SWITCHPA4 D1    // Pin de accionamiento de Relé en TB para lectura de PA4
#define RELAYPA4_ON D0  // Pin de suministro de 3.3V al relé de PA4

// ==== Inicialización de objetos
HardwareSerial PagWeb(1);  // Objeto para UART2 en PULSAR como PagWeb

// MapController class with full functionality (MapCRUD alias available for command compatibility)
MapController deviceManager;

// ==== Variables de inicialización JSON
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

// Global variables for demo Manager
unsigned long lastTestTime = 0;
bool demoMode = false;
int demoStep = 0;

// Function declarations
void showBanner();
void showMenu();
void processCommand(String cmd);
void runDemo();
void printDeviceStatus();



void pagwebDebug(String str) {
  sendJSON.clear();
  sendJSON["debug"] = str;
  serializeJson(sendJSON, PagWeb);
  PagWeb.println();
}


void setup() {

  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);  // Iniciar UART2 en los pines seleccionados
  Serial.println("UART2 iniciado en RX=9, TX=8");

  Wire.setTimeOut(50);

  // Declaración de pines de entrada de relevador
  pinMode(RELAYNO, INPUT);
  pinMode(RELAYNC, INPUT);
  pinMode(RUN_BUTTON, INPUT_PULLDOWN);

  pinMode(SWITCHPA4, OUTPUT);
  pinMode(RELAYPA4_ON, OUTPUT);
  digitalWrite(SWITCHPA4, LOW);
  digitalWrite(RELAYPA4_ON, HIGH);

  //showBanner();

  deviceManager.begin(Wire, I2C_SDA, I2C_SCL, 200000);  // ESP32: SDA=21, SCL=22, 100kHz
  deviceManager.initializeI2C(1);                       // print=1 to show initialization

  // Initialize device mapping system
  deviceManager.initializeMapping(1);

  // Perform initial scan
  //Serial.println(">> Realizando scan inicial automatico...");
  //deviceManager.scanDevices(1);
}


void loop() {

  /*
  // Handle continuous testing if active
  if (deviceManager.isContinuousTestActive()) {
    deviceManager.runContinuousTest();
    delay(deviceManager.getTestInterval());
  }

  // Run demo mode if active
  if (demoMode) {
    runDemo();
  }
  */

  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();
    delay(100);
    if (digitalRead(RUN_BUTTON) == LOW) {
      Serial.println(">> Accionamiento por botonera");
      sendJSON["Run"] = "OK";
      serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
      PagWeb.println();
    }
  }

  // Process user commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd.length() > 0) {
      Serial.printf("CMD: %s\n", cmd.c_str());
      processCommand(cmd);
      Serial.print("\nMapController> ");
    }
  }


  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {

      String Function = receiveJSON["Function"];   // Function es la variable de interés del JSON
      String addressStr = receiveJSON["Address"];  // "0x40"
      uint8_t address = (uint8_t)strtol(addressStr.c_str(), NULL, 16);

      int opc = 0;

      if (Function == "ping") opc = 1;            // {"Function":"ping"}
      else if (Function == "scan") opc = 2;       // {"Function":"scan"}
      else if (Function == "uid") opc = 3;        // {"Function":"uid", "Address": "0X42"}
      else if (Function == "TestBuz") opc = 4;    // {"Function":"TestBuz", "Address": "0X42"}
      else if (Function == "TestRelay") opc = 5;  // {"Function":"TestRelay", "Address": "0X42"}
      else if (Function == "TestNeo") opc = 6;    // {"Function":"TestNeo", "Address": "0X42"}
      else if (Function == "TestAll") opc = 7;    // {"Function":"TestAll", "Address": "0X42"}

      switch (opc) {

        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }

        case 2:  // Escaneo de dirección I2C con Timeout
          {
            sendJSON.clear();  // Limpia cualquier dato previo
            String addrStr = "null";
            String status = "FAIL";

            unsigned long timeout_ms = 3000;  // Tiempo máximo de espera (3 segundos)
            unsigned long startTime = millis();

            // Bucle de reintentos basado en tiempo
            while ((millis() - startTime) < timeout_ms) {

              String resultado = deviceManager.scanDevices(1);
              addrStr = scanDirection(resultado);

              // Solo declaramos OK si addrStr NO es "null" y NO está vacío
              if (addrStr != "null" && addrStr != "") {
                status = "OK";
                break;  // Rompemos el bucle while
              }

              // Pequeña pausa antes de volver a preguntar por I2C
              delay(100);
            }

            // Guardamos el resultado final (ya sea la dirección o el FAIL por timeout)
            sendJSON["Ad"] = addrStr;         // Dirección I2C o "null"
            sendJSON["Result"] = status;      // "OK" o "FAIL"
            serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
            PagWeb.println();
            break;
          }

        case 3:
          {
            sendJSON.clear();
            if (address > 0) {
              // 1. Guardamos lo que devuelve la función en un String
              String resultadoUID = deviceManager.showDeviceUID(address, 1);

              // 2. Usamos una función nueva para parsear/extraer el valor
              String uid_extraido = extraerUID(resultadoUID);

              if (uid_extraido != "FAIL") {
                sendJSON["uid"] = uid_extraido;  // Guardará "0xF1F0F5F5"
                sendJSON["status"] = "OK";
                sendJSON["ping"] = "pong";
              } else {
                sendJSON["uid"] = "NOT_FOUND";
                sendJSON["status"] = "FAIL";
              }
            } else {
              Serial.println("UID no identificada");  // O tu printDebug
              sendJSON["uid"] = "INVALID_ADDR";
              sendJSON["status"] = "FAIL";
            }

            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }


        case 4:  // Ejecución de prueba única de buzzer
          {
            int delay_ms = 150;
            deviceManager.cmdPWM25(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM50(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM75(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM100(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM75(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM50(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM25(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWMOff(address, 1);
            break;
          }

        case 5:  // Ejecución de prueba única de Relay
          {
            int iteraciones = 10;
            int delay_ms = 500;
            for (int i = 0; i < iteraciones; i++) {
              deviceManager.cmdRelay(address, true, 1);  // Accionamiento
              delay(delay_ms);
              deviceManager.cmdRelay(address, false, 1);
              delay(delay_ms);
            }
            deviceManager.cmdRelay(address, false, 1);
            break;
          }


        case 6:  // Ejecución de prueba única de Neopixel
          {
            int delay_ms = 1200;
            deviceManager.cmdNeoRed(address, 1);
            delay(delay_ms);
            deviceManager.cmdNeoBlue(address, 1);
            delay(delay_ms);
            deviceManager.cmdNeoGreen(address, 1);
            delay(delay_ms);
            deviceManager.cmdNeoWhite(address, 1);
            delay(delay_ms);
            deviceManager.cmdNeoOff(address, 1);
            delay(delay_ms);
            ESP.restart();
            break;
          }

        case 7:  // Ejecución de Test Completo
          {
            // ==== Bloque de Relay ====
            bool statusRele = false;
            deviceManager.cmdRelay(address, false, 1);  // Accionamiento
            delay(500);
            float relayNC_init = avgAnalog(RELAYNC, 10);
            delay(100);
            float relayNO_init = avgAnalog(RELAYNO, 10);
            Serial.println("Valor init NC: " + String(relayNC_init));
            Serial.println("Valor init NO: " + String(relayNO_init));
            Serial.println("");

            deviceManager.cmdRelay(address, true, 1);  // Accionamiento
            delay(2000);
            float relayNC_fin = avgAnalog(RELAYNC, 10);
            delay(100);
            float relayNO_fin = avgAnalog(RELAYNO, 10);
            Serial.println("Valor fin NC: " + String(relayNC_fin));
            Serial.println("Valor fin NO: " + String(relayNO_fin));
            deviceManager.cmdRelay(address, false, 1);  // Accionamiento

            if (fabs(relayNC_init - relayNC_fin) > 2200 && fabs(relayNO_init - relayNO_fin) > 2200) {
              statusRele = true;
            }

            JsonObject relayObj = sendJSON.createNestedObject("Relay");
            relayObj["status"] = statusRele;
            relayObj["nc_init"] = relayNC_init;
            relayObj["nc_fin"] = relayNC_fin;
            relayObj["no_init"] = relayNO_init;
            relayObj["no_fin"] = relayNO_fin;

            // ==== Bloque de GPIO ADC ====
            bool statusADC = false;
            digitalWrite(SWITCHPA4, HIGH);

            String resultado1 = deviceManager.cmdReadPA4(address, 1);  // Lectura inicial HIGH
            int pos1 = resultado1.lastIndexOf('(');
            int valor1 = resultado1.substring(pos1 + 1, pos1 + 2).toInt();  // valor = 1

            digitalWrite(SWITCHPA4, LOW);
            delay(500);

            String resultado2 = deviceManager.cmdReadPA4(address, 1);  // Lectura final LOW
            int pos2 = resultado2.lastIndexOf('(');
            int valor2 = resultado2.substring(pos2 + 1, pos2 + 2).toInt();  // valor = 1
            digitalWrite(SWITCHPA4, HIGH);

            if (valor1 != valor2) {
              statusADC = true;
            }

            // --- CREACIÓN DEL OBJETO ANIDADO EN EL JSON ---
            JsonObject adcObj = sendJSON.createNestedObject("ADC");
            adcObj["status"] = statusADC;
            adcObj["value1"] = valor1;
            adcObj["value2"] = valor2;

            serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
            PagWeb.println();

            // ==== Bloque de Buzzer ====
            int delay_ms = 200;
            deviceManager.cmdPWM25(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM50(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM75(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM100(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM75(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM50(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWM25(address, 1);
            delay(delay_ms);
            deviceManager.cmdPWMOff(address, 1);
            delay(delay_ms);

            // ==== Bloque de Neopixel ====
            delay_ms = 1500;
            deviceManager.cmdNeoRed(address, 1);
            delay(delay_ms);
            deviceManager.cmdNeoBlue(address, 1);
            delay(delay_ms);
            deviceManager.cmdNeoGreen(address, 1);
            delay(delay_ms);
            deviceManager.cmdNeoWhite(address, 1);
            delay(delay_ms);
            deviceManager.cmdNeoOff(address, 1);
            delay(delay_ms);
            //ESP.restart();
            break;
          }

        default: break;
      }
    }
  }
  delay(500);
}


String scanDirection(String resultado) {
  int okPos = resultado.indexOf("[OK]");
  if (okPos != -1) {
    // Buscar "0x" SOLO después de la línea [OK]
    int dirPos = resultado.indexOf("0x", okPos);
    if (dirPos != -1) {
      // Buscar el siguiente espacio o salto de línea
      int fin = resultado.indexOf(' ', dirPos);
      if (fin == -1) fin = resultado.indexOf('\n', dirPos);
      if (fin == -1) fin = resultado.length();  // Por si es el último caracter

      String address = resultado.substring(dirPos, fin);
      address.trim();  // Limpiar saltos de línea (\r\n) invisibles

      Serial.println("Direccion encontrada: " + address);
      return address;
    }
  }

  // Si no hay [OK] o no hay "0x", estandarizamos la salida de error a "null"
  return "null";
}

String extraerUID(String resultado) {
  // Buscamos la etiqueta exacta que imprime el monitor serie
  String etiqueta = "UID (32-bit): ";
  int posInicial = resultado.indexOf(etiqueta);

  if (posInicial != -1) {
    // Calculamos dónde empieza el valor hexadecimal
    int inicioValor = posInicial + etiqueta.length();

    // Buscamos el salto de línea que marca el final de esa fila
    int finValor = resultado.indexOf('\n', inicioValor);
    if (finValor == -1) finValor = resultado.length();  // Por si es la última línea

    // Recortamos el valor y lo limpiamos de espacios o retornos de carro (\r)
    String uidStr = resultado.substring(inicioValor, finValor);
    uidStr.trim();

    return uidStr;
  }

  return "FAIL";
}


void printDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  Serial.println("{\"debug\": \"" + str + "\"}");
}

float avgAnalog(uint8_t PIN, int iter) {

  float avg = 0;
  float value = 0;

  for (int i = 0; i < iter; i++) {
    value = analogRead(PIN);
    Serial.println("Lectura: " + String(value));
    avg += value;
    delay(50);
  }

  return avg / iter;
}


void showBanner() {
  Serial.println("\n===========================================");
  Serial.println("    MAP CONTROLLER - Complete Example     ");
  Serial.println("   I2C Device Manager & Testing System   ");
  Serial.println("  Compatible with PY32F003 v6.0.0_lite  ");
  Serial.println("===========================================");
  Serial.println("Features:");
  Serial.println("✓ Device scanning & compatibility check");
  Serial.println("✓ Continuous testing with statistics");
  Serial.println("✓ I2C address management");
  Serial.println("✓ PWM/Buzzer control + Musical tones 🎵");
  Serial.println("✓ System alerts (SUCCESS/OK/WARN/ALERT) 🔔");
  Serial.println("✓ NeoPixel control");
  Serial.println("✓ Sensor reading (PA4, ADC)");
  Serial.println("✓ Device information & UID");
  Serial.println("✓ Security: Relay commands 0xA0-0xAF ⚠️");
  Serial.println("===========================================\n");
}

void showMenu() {
  Serial.println("\nCOMANDOS DISPONIBLES:");
  Serial.println("===============================================");
  Serial.println("=== GESTION BASICA ===");
  Serial.println("'scan' / 's'        = Escanear dispositivos I2C");
  Serial.println("'list' / 'l'        = Ver dispositivos encontrados");
  Serial.println("'ping 0x20'         = Ping a dispositivo especifico");
  Serial.println("'stats'             = Ver estadisticas detalladas");
  Serial.println("'menu' / 'm'        = Mostrar este menu");
  Serial.println("");
  Serial.println("=== DEMO & TESTING ===");
  Serial.println("'demo'              = Iniciar modo demo automatico");
  Serial.println("'stop-demo'         = Detener modo demo");
  Serial.println("'test-start pa4'    = Test continuo lectura PA4");
  Serial.println("'test-start toggle' = Test continuo toggle");
  Serial.println("'test-start neo'    = Test continuo NeoPixels");
  Serial.println("'test-start pwm'    = Test continuo PWM/buzzer");
  Serial.println("'test-start music'  = 🎵 Test escala musical (v6.0.0)");
  Serial.println("'test-start alerts' = 🔔 Test alertas sistema (v6.0.0)");
  Serial.println("'test-start gradual'= 📈 Test PWM gradual (v6.0.0)");
  Serial.println("'test-stop'         = Detener test continuo");
  Serial.println("'interval 1000'     = Cambiar intervalo test (ms)");
  Serial.println("");
  Serial.println("=== CONTROL DE DISPOSITIVOS ===");
  Serial.println("'relay 0x20 on'     = Encender rele");
  Serial.println("'relay 0x20 off'    = Apagar rele");
  Serial.println("'toggle 0x20'       = Pulso de rele (toggle)");
  Serial.println("'neo 0x20 red'      = NeoPixels rojos");
  Serial.println("'neo 0x20 white'    = NeoPixels blancos");
  Serial.println("'neo 0x20 off'      = Apagar NeoPixels");
  Serial.println("'pwm 0x20 25'       = PWM 25% (200Hz)");
  Serial.println("'pwm 0x20 100'      = PWM 100% (2kHz)");
  Serial.println("'pwm 0x20 off'      = PWM silencio");
  Serial.println("'tone 0x20 do'      = 🎵 Tocar nota DO (261Hz)");
  Serial.println("'tone 0x20 la'      = 🎵 Tocar nota LA (440Hz)");
  Serial.println("'alert 0x20 ok'     = 🔔 Alerta OK (1000Hz)");
  Serial.println("'alert 0x20 warn'   = ⚠️ Alerta WARNING (1200Hz)");
  Serial.println("");
  Serial.println("=== SENSORES ===");
  Serial.println("'pa4 0x20'          = Leer entrada digital PA4");
  Serial.println("'adc 0x20'          = Leer ADC PA0");
  Serial.println("");
  Serial.println("=== INFORMACION ===");
  Serial.println("'info 0x20'         = Informacion del dispositivo");
  Serial.println("'uid 0x20'          = UID unico del dispositivo");
  Serial.println("'test-all 0x20'     = Test completo de comandos");
  Serial.println("'test-both 0x20'    = Test rápido (solo toggle+pa4)");
  Serial.println("'test-both 0x20'    = Test rápido (solo toggle + pa4)");
  Serial.println("");
  Serial.println("=== GESTION I2C ===");
  Serial.println("'change 0x20 0x30'  = Cambiar direccion I2C");
  Serial.println("'factory 0x20'      = Factory reset a UID");
  Serial.println("'reset 0x20'        = Reiniciar dispositivo");
  Serial.println("'status 0x20'       = Estado I2C (Flash/UID)");
  Serial.println("===============================================");
  Serial.println("Ejemplos:");
  Serial.println("  scan               (escanear bus)");
  Serial.println("  demo               (modo demostracion)");
  Serial.println("  test-start toggle  (test automatico)");
  Serial.println("  relay 0x20 on      (encender rele)");
  Serial.println("  neo 0x20 blue      (NeoPixels azules)");
  Serial.println("===============================================");
}

void processCommand(String cmd) {
  // Basic management commands
  if (cmd == "scan" || cmd == "s") {
    deviceManager.scanDevices(1);

  } else if (cmd == "list" || cmd == "l") {
    deviceManager.listDevices(1);

  } else if (cmd == "menu" || cmd == "m") {
    showMenu();

  } else if (cmd == "stats") {
    deviceManager.showStatistics(1);

  } else if (cmd.startsWith("ping ")) {
    String addr_str = cmd.substring(5);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.pingDevice(address, 1);
    }

    // Demo and testing commands
  } else if (cmd == "demo") {
    demoMode = true;
    demoStep = 0;
    Serial.println("[OK] Modo demo iniciado. Usa 'stop-demo' para detener.");

  } else if (cmd == "stop-demo") {
    demoMode = false;
    Serial.println("[OK] Modo demo detenido.");

  } else if (cmd == "test-start pa4") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_PA4_ONLY);
    Serial.println("[OK] Test continuo PA4 iniciado");

  } else if (cmd == "test-start toggle") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_TOGGLE_ONLY);
    Serial.println("[OK] Test continuo toggle iniciado");

  } else if (cmd == "test-start neo") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_NEO_CYCLE);
    Serial.println("[OK] Test continuo NeoPixels iniciado");

  } else if (cmd == "test-start pwm") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_PWM_TONES);
    Serial.println("[OK] Test continuo PWM iniciado");

  } else if (cmd == "test-start music") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_MUSICAL_SCALE);
    Serial.println("[OK] 🎵 Test escala musical DO-SI iniciado (v6.0.0_lite)");

  } else if (cmd == "test-start alerts") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_ALERTS);
    Serial.println("[OK] 🔔 Test alertas sistema iniciado (v6.0.0_lite)");

  } else if (cmd == "test-start gradual") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_PWM_GRADUAL);
    Serial.println("[OK] 📈 Test PWM gradual 100Hz-1000Hz iniciado (v6.0.0_lite)");

  } else if (cmd == "test-stop") {
    deviceManager.stopContinuousTest();
    Serial.println("[OK] Test continuo detenido");

  } else if (cmd.startsWith("interval ")) {
    String interval_str = cmd.substring(9);
    uint32_t new_interval = interval_str.toInt();
    deviceManager.setTestInterval(new_interval);
    Serial.printf("[OK] Intervalo cambiado a %d ms\n", new_interval);

    // Device control commands
  } else if (cmd.startsWith("relay ")) {
    parseRelayCommand(cmd);

  } else if (cmd.startsWith("toggle ")) {
    String addr_str = cmd.substring(7);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.cmdToggle(address, 1);
    }

  } else if (cmd.startsWith("neo ")) {
    parseNeoCommand(cmd);

  } else if (cmd.startsWith("pwm ")) {
    parsePWMCommand(cmd);

  } else if (cmd.startsWith("tone ")) {
    parseToneCommand(cmd);

  } else if (cmd.startsWith("alert ")) {
    parseAlertCommand(cmd);

    // Sensor commands
  } else if (cmd.startsWith("pa4 ")) {
    String addr_str = cmd.substring(4);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.cmdReadPA4(address, 1);
    }

  } else if (cmd.startsWith("adc ")) {
    String addr_str = cmd.substring(4);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.cmdReadADC(address, 1);
    }

    // Information commands
  } else if (cmd.startsWith("info ")) {
    String addr_str = cmd.substring(5);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.showDeviceInfo(address, 1);
    }

  } else if (cmd.startsWith("uid ")) {
    String addr_str = cmd.substring(4);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.showDeviceUID(address, 1);
    }

  } else if (cmd.startsWith("test-all ")) {
    String addr_str = cmd.substring(9);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.testAllCommands(address, 1);
    }

  } else if (cmd.startsWith("test-both ")) {
    String addr_str = cmd.substring(10);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.testBothCommands(address, 1);
    }

  } else if (cmd.startsWith("test-both ")) {
    String addr_str = cmd.substring(10);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.testBothCommands(address, 1);
    }

    // I2C management commands
  } else if (cmd.startsWith("change ")) {
    parseChangeCommand(cmd);

  } else if (cmd.startsWith("factory ")) {
    String addr_str = cmd.substring(8);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.factoryReset(address, 1);
    }

  } else if (cmd.startsWith("reset ")) {
    String addr_str = cmd.substring(6);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.resetDevice(address, 1);
    }

  } else if (cmd.startsWith("status ")) {
    String addr_str = cmd.substring(7);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.checkI2CStatus(address, 1);
    }

  } else if (cmd.length() > 0) {
    Serial.println("ERROR: Comando no reconocido. Usa 'menu' para ver comandos disponibles.");
  }
}

void parseRelayCommand(String cmd) {
  // Parse: "relay 0x20 on/off"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: relay <address> on/off");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String action = cmd.substring(second_space + 1);
  action.toLowerCase();

  uint8_t address = deviceManager.parseAddress(addr_str);
  if (address == 0) return;

  if (action == "on") {
    deviceManager.cmdRelay(address, true, 1);
  } else if (action == "off") {
    deviceManager.cmdRelay(address, false, 1);
  } else {
    Serial.println("ERROR: Usar 'on' o 'off'");
  }
}

void parseNeoCommand(String cmd) {
  // Parse: "neo 0x20 red/green/blue/white/off"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: neo <address> red/green/blue/white/off");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String color = cmd.substring(second_space + 1);
  color.toLowerCase();

  uint8_t address = deviceManager.parseAddress(addr_str);
  if (address == 0) return;

  if (color == "red") {
    deviceManager.cmdNeoRed(address, 1);
  } else if (color == "green") {
    deviceManager.cmdNeoGreen(address, 1);
  } else if (color == "blue") {
    deviceManager.cmdNeoBlue(address, 1);
  } else if (color == "white") {
    deviceManager.cmdNeoWhite(address, 1);
  } else if (color == "off") {
    deviceManager.cmdNeoOff(address, 1);
  } else {
    Serial.println("ERROR: Usar red/green/blue/white/off");
  }
}

void parsePWMCommand(String cmd) {
  // Parse: "pwm 0x20 off/25/50/75/100"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: pwm <address> off/25/50/75/100");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String level = cmd.substring(second_space + 1);
  level.toLowerCase();

  uint8_t address = deviceManager.parseAddress(addr_str);
  if (address == 0) return;

  if (level == "off" || level == "0") {
    deviceManager.cmdPWMOff(address, 1);
  } else if (level == "25") {
    deviceManager.cmdPWM25(address, 1);
  } else if (level == "50") {
    deviceManager.cmdPWM50(address, 1);
  } else if (level == "75") {
    deviceManager.cmdPWM75(address, 1);
  } else if (level == "100") {
    deviceManager.cmdPWM100(address, 1);
  } else {
    Serial.println("ERROR: Usar off/25/50/75/100");
  }
}

void parseToneCommand(String cmd) {
  // Parse: "tone 0x20 do/re/mi/fa/sol/la/si"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: tone <address> do/re/mi/fa/sol/la/si");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String note = cmd.substring(second_space + 1);
  note.toLowerCase();

  uint8_t address = deviceManager.parseAddress(addr_str);
  if (address == 0) return;

  if (note == "do") {
    deviceManager.cmdToneDo(address, 1);
  } else if (note == "re") {
    deviceManager.cmdToneRe(address, 1);
  } else if (note == "mi") {
    deviceManager.cmdToneMi(address, 1);
  } else if (note == "fa") {
    deviceManager.cmdToneFa(address, 1);
  } else if (note == "sol") {
    deviceManager.cmdToneSol(address, 1);
  } else if (note == "la") {
    deviceManager.cmdToneLa(address, 1);
  } else if (note == "si") {
    deviceManager.cmdToneSi(address, 1);
  } else {
    Serial.println("ERROR: Usar do/re/mi/fa/sol/la/si");
  }
}

void parseAlertCommand(String cmd) {
  // Parse: "alert 0x20 success/ok/warn/critical"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: alert <address> success/ok/warn/critical");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String alert = cmd.substring(second_space + 1);
  alert.toLowerCase();

  uint8_t address = deviceManager.parseAddress(addr_str);
  if (address == 0) return;

  if (alert == "success") {
    deviceManager.cmdSuccess(address, 1);
  } else if (alert == "ok") {
    deviceManager.cmdOk(address, 1);
  } else if (alert == "warn" || alert == "warning") {
    deviceManager.cmdWarning(address, 1);
  } else if (alert == "critical" || alert == "alert") {
    deviceManager.cmdAlert(address, 1);
  } else {
    Serial.println("ERROR: Usar success/ok/warn/critical");
  }
}

void parseChangeCommand(String cmd) {
  // Parse: "change 0x20 0x30"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: change <old_address> <new_address>");
    return;
  }

  String old_str = cmd.substring(first_space + 1, second_space);
  String new_str = cmd.substring(second_space + 1);

  uint8_t old_addr = deviceManager.parseAddress(old_str);
  uint8_t new_addr = deviceManager.parseAddress(new_str);

  if (old_addr > 0 && new_addr > 0) {
    deviceManager.changeI2CAddress(old_addr, new_addr, 1);
  }
}

void runDemo() {
  static unsigned long lastDemoTime = 0;

  if (millis() - lastDemoTime < 3000) return;  // 3 second intervals
  lastDemoTime = millis();

  if (deviceManager.getDeviceCount() == 0) {
    Serial.println("[DEMO] No hay dispositivos para demostrar");
    demoMode = false;
    return;
  }

  // Get first compatible device
  const MapController::DeviceInfo* devices = deviceManager.getDevices();
  uint8_t demo_addr = 0;

  for (int i = 0; i < deviceManager.getDeviceCount(); i++) {
    if (devices[i].is_compatible) {
      demo_addr = devices[i].address;
      break;
    }
  }

  if (demo_addr == 0) {
    Serial.println("[DEMO] No hay dispositivos compatibles para demostrar");
    demoMode = false;
    return;
  }

  Serial.printf("[DEMO] Paso %d - Dispositivo 0x%02X\n", demoStep + 1, demo_addr);

  switch (demoStep % 8) {
    case 0:
      Serial.println("[DEMO] NeoPixels rojos");
      deviceManager.cmdNeoRed(demo_addr, 1);
      break;
    case 1:
      Serial.println("[DEMO] NeoPixels verdes");
      deviceManager.cmdNeoGreen(demo_addr, 1);
      break;
    case 2:
      Serial.println("[DEMO] NeoPixels azules");
      deviceManager.cmdNeoBlue(demo_addr, 1);
      break;
    case 3:
      Serial.println("[DEMO] NeoPixels blancos");
      deviceManager.cmdNeoWhite(demo_addr, 1);
      break;
    case 4:
      Serial.println("[DEMO] NeoPixels OFF");
      deviceManager.cmdNeoOff(demo_addr, 1);
      break;
    case 5:
      Serial.println("[DEMO] Toggle rele");
      deviceManager.cmdToggle(demo_addr, 1);
      break;
    case 6:
      Serial.println("[DEMO] Lectura PA4");
      deviceManager.cmdReadPA4(demo_addr, 1);
      break;
    case 7:
      Serial.println("[DEMO] Lectura ADC");
      deviceManager.cmdReadADC(demo_addr, 1);
      break;
  }

  demoStep++;
}



void showHelp() {
  Serial.println("\n╔══════════════════════════════════════════════════════════════╗");
  Serial.println("║                    FAST COMMANDS (≤2 chars)                 ║");
  Serial.println("╚══════════════════════════════════════════════════════════════╝");
  Serial.println("⚡ SYSTEM FAST:");
  Serial.println("  sc = scan       m = map         am = automap    h = help");
  Serial.println("  c = clear       r = reboot");
  Serial.println("⚡ ADC FAST:");
  Serial.println("  a1 = adc 1      a2 = adc 2      a3 = adc 3      a4 = adc 4");
  Serial.println("  a5 = adc 5      aa = adc all");
  Serial.println("⚡ RELAY FAST:");
  Serial.println("  f1 = fire 1     f2 = fire 2     f3 = fire 3     f4 = fire 4");
  Serial.println("  f5 = fire 5     ro = relay off all");
  Serial.println("⚡ NEOPIXEL FAST:");
  Serial.println("  nr = red all    ng = green all  nb = blue all   no = off all");
  Serial.println("⚡ FLASH DEBUG FAST (PRIORITY):");
  Serial.println("  fs <id> <data> = flash save     fr <id> = flash read");
  Serial.println("  Example: fs 1 25, fs 1 0x25, fr 1");
  Serial.println("⚡ ARRAY CONTROL FAST (NEW):");
  Serial.println("  sa <n> = sense array n          sh <n> = show array n");
  Serial.println("  fa <n> = fill array n (TEST!)   all = show all arrays");
  Serial.println("  Example: sa 0, sh 1, fa 2, all (n = 0-3)");
  Serial.println("⚡ I2C MANAGEMENT FAST:");
  Serial.println("  si <id> <addr> = set i2c        rf <id> = reset factory");
  Serial.println("  is <id> = i2c status            dr <id> = debug reset");
  Serial.println("  Example: si 1 25, rf 1, is 1, dr 1");

  Serial.println("\n╔══════════════════════════════════════════════════════════════╗");
  Serial.println("║                    STANDARD COMMANDS                        ║");
  Serial.println("╚══════════════════════════════════════════════════════════════╝");
  Serial.println("SYSTEM:");
  Serial.println("  scan, s         - Scan for I2C devices");
  Serial.println("  reboot          - Reboot the ESP32");
  Serial.println("  help, h, ?      - Show this help");
  Serial.println("  clear, cls      - Clear screen");
  Serial.println("\nMAPPING (CRUD):");
  Serial.println("  map, ls         - (List) Show current ID-address mapping");
  Serial.println("  automap         - (Create) Auto-assign scanned devices to free IDs");
  Serial.println("  assign <id> <addr> - (Create/Update) Assign an address to an ID");
  Serial.println("  mv <from> <to>  - (Update) Move/Swap a device from one ID to another");
  Serial.println("  rm <id>         - (Delete) Remove a device from an ID");
  Serial.println("  clear map       - (Delete) Clear all mappings from memory and storage");
  Serial.println("  save            - Save current mapping to persistent storage");
  Serial.println("  load            - Load mapping from persistent storage");
  Serial.println("\nSENSE & RELAY CONTROL (uses ID mapping):");
  Serial.println("  sense           - Read PA4 state from all mapped devices");
  Serial.println("  mask            - Get 4-byte bitmask of PA4 states for mapped devices");
  Serial.println("  sense_array <n> - Get detailed mask for array n (0-3: IDs 1-8, 9-16, 17-24, 25-32)");
  Serial.println("  show_array <n>  - Show detailed view of array n with bit positions");
  Serial.println("  fill_array <n>  - Test fill array n (briefly activates relays - USE WITH CARE!)");
  Serial.println("  show_all        - Overview of all 4 arrays with their current masks");
  Serial.println("  f <id>          - Send momentary relay pulse to a device by ID");
  Serial.println("  t <id>          - Toggle relay state on a device by ID");
  Serial.println("  roff            - Turn off all mapped relays");
  Serial.println("  shut            - Shutdown all mapped devices (LEDs and relays off)");
  Serial.println("\nNEOPIXEL CONTROL (uses ID mapping):");
  Serial.println("  neo red [id]    - Set NeoPixel(s) to red");
  Serial.println("  neo green [id]  - Set NeoPixel(s) to green");
  Serial.println("  neo blue [id]   - Set NeoPixel(s) to blue");
  Serial.println("  neo off [id]    - Turn off NeoPixel(s)");
  Serial.println("  red, green, blue, off - Quick color commands for all");
  Serial.println("\nADC READING (uses ID mapping - 12-bit, 3.3V reference):");
  Serial.println("  adc <id>        - Read ADC value from specific device by ID");
  Serial.println("  adcall, aa      - Read ADC values from all mapped devices");
  Serial.println("  Note: ADC is 12-bit (0-4095) with 3.3V reference, automatically masked");
  Serial.println("\nDEBUG FLASH COMMANDS (NEW):");
  Serial.println("  flash_save <id> <data> - Save data byte to device flash (hex or decimal)");
  Serial.println("  flash_read <id>        - Read data byte from device flash");
  Serial.println("  Examples: flash_save 1 0x25, flash_save 1 37, flash_read 1");
  Serial.println("\nI2C ADDRESS MANAGEMENT (NEW):");
  Serial.println("  set_i2c <id> <addr>    - Change device I2C address (0x08-0x77)");
  Serial.println("  reset_factory <id>     - Reset device to factory I2C address");
  Serial.println("  i2c_status <id>        - Get device I2C configuration status");
  Serial.println("  Examples: set_i2c 1 0x25, reset_factory 1, i2c_status 1");
  Serial.println("  -----Address changes require device restart (~2s)");
  Serial.println("\nDIAGNOSTIC COMMANDS:");
  Serial.println("  testcmds        - Test all command values (0x00-0xFF) to find supported ones");
  Serial.println("════════════════════════════════════════════════════════════════");
}
