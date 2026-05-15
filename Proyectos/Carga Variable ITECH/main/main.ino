/*
 * ============================================================================
 * FIRMWARE: PUENTE MODBUS ITECH LOAD - PULSAR C6
 * ============================================================================
 * 
 * DESCRIPCIÓN:
 * Este firmware actúa como puente de comunicación entre una tarjeta Pulsar C6
 * y cargas variables ITECH. Permite controlar modos de operación específicos:
 * - Dinámico (DYN): Alterna entre dos valores de corriente en intervalos
 * - Lista (LIST): Ejecuta una secuencia de pasos de corriente programados
 * 
 * CONEXIONES:
 * - USB (Serial Nativo) <-> PC (Control del firmware)
 * - GPIO 4 (RX1/DB9) <-> Carga ITECH
 * - GPIO 5 (TX1/DB9) <-> Carga ITECH
 * - Baudrate: 9600 bps (ambas comunicaciones)
 * 
 * FLUJO GENERAL:
 * PC envía JSON -> Pulsar procesa -> Configura carga ITECH -> Entrega respuesta JSON
 * 
 * ============================================================================
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// ============================================================================
// CONFIGURACIÓN DE PINES UART PARA CARGA ITECH
// ============================================================================
#define RX1 4  // GPIO 4: Pin de recepción desde carga ITECH
#define TX1 5  // GPIO 5: Pin de transmisión hacia carga ITECH

// Crear instancia de Serial adicional para comunicación con ITECH (UART1)
HardwareSerial ITECH(1);

// ============================================================================
// VARIABLES PARA MODO LIST (LISTA DE PASOS)
// ============================================================================
// Estructura que almacena cada paso: corriente y duración
struct Step {
  float current;   // Valor de corriente a aplicar
  float duration;  // Duración en segundos
};

Step softwareList[30];     // Array con máximo 30 pasos
int totalSteps = 0;        // Contador de pasos a ejecutar
bool isListReady = false;  // Bandera: ¿está la lista lista para ejecutar?

// ============================================================================
// VARIABLES PARA MODO DINÁMICO (ALTERNANCIA DE CORRIENTE)
// ============================================================================
float dynA, dynB;                  // Valores de corriente A y B
unsigned long dynTimeA, dynTimeB;  // Duración en ms para cada estado
unsigned long lastDynMillis = 0;   // Marca de tiempo anterior
bool dynState = false;             // Estado actual (false=A, true=B)
bool runDynamic = false;           // Bandera: ¿está el modo dinámico activo?

int numSamples = 1;  // Número de muestras para medición

// ============================================================================
// FUNCIÓN: sendSCPI()
// ============================================================================
// PROPÓSITO:
// Envía comandos SCPI (Standar Commands for Programmable Instruments) a la
// carga ITECH por comunicación serial UART1. El delay de 45ms permite que
// la carga procese el comando antes de enviar el siguiente.
//
// PARÁMETROS:
// - cmd: Comando SCPI a enviar (ejemplo: "FUNC CURRent", "INP 1")
//
// RESPUESTA:
// No retorna respuesta directa. La carga responde en el buffer ITECH.
//
// EJEMPLOS:
// - sendSCPI("INP 1");              // Activar entrada (carga ON)
// - sendSCPI("INP 0");              // Desactivar entrada (carga OFF)
// - sendSCPI("CURRent 5.5");        // Establecer corriente a 5.5 A
// - sendSCPI("MEAS:VOLT?;CURR?");   // Solicitar mediciones
// ============================================================================
void sendSCPI(String cmd) {
  ITECH.println(cmd);
  delay(45);  // Esperar que la carga ITECH procese el comando
}



// ============================================================================
// FUNCIÓN: takeMeasurements()
// ============================================================================
// PROPÓSITO:
// Lee mediciones instantáneas de la carga ITECH: tensión, corriente y potencia.
// Formato de salida: "V;I;P" con 3 decimales de precisión.
//
// PROCESO INTERNO:
// 1. Solicita a la carga: MEAS:VOLT?;CURR?;POW? (voltaje, corriente, potencia)
// 2. Espera respuesta máximo 300ms
// 3. Procesa la respuesta y la formatea a JSON
// 4. Envía al PC por USB el resultado
//
// RESPUESTA TÍPICA:
// {
//   "medicion": "12.345;2.890;35.678"
//          └─────────────────────────
//               ↓         ↓       ↓
//            Voltios Amperios Watios
// }
//
// NOTAS:
// - Usa sscanf para robustez contra diferentes formatos de la carga
// - Si falla el parseo, reemplaza tabuladores por punto y coma
// - Precisión: 3 decimales (ejemplo: 12.345 V)
// ============================================================================
void takeMeasurements() {
  DynamicJsonDocument res(512);
  res.clear();                                 // Limpieza de objeto JSON
  String valorMedicion = "0.000;0.000;0.000";  // Limpieza de variable de medición

  // Solicitar las 3 mediciones a la carga
  ITECH.println("MEAS:VOLT?;CURR?;POW?");
  unsigned long t = millis();

  // Esperar respuesta con timeout de 300ms
  while (!ITECH.available() && (millis() - t < 300))
    ;

  if (ITECH.available()) {
    String raw = ITECH.readStringUntil('\n');
    raw.trim();  // Eliminar espacios al inicio/final

    float v = 0.0, c = 0.0, p = 0.0;
    // sscanf intenta extraer los 3 valores flotantes de la cadena
    int parseados = sscanf(raw.c_str(), "%f %f %f", &v, &c, &p);

    if (parseados == 3) {
      // Si se parsearon correctamente: formatear con 3 decimales exactos
      valorMedicion = String(v, 3) + ";" + String(c, 3) + ";" + String(p, 3);
    } else {
      // Si sscanf falla (formato diferente): reemplazar tabuladores
      raw.replace("\t", ";");
      valorMedicion = raw;
    }
  }

  // Enviar resultado JSON al PC
  res["medicion"] = valorMedicion;
  serializeJson(res, Serial);
  Serial.println();
}



// ============================================================================
// FUNCIÓN: configLoad()
// ============================================================================
// PROPÓSITO:
// Configura la carga ITECH en modo LISTA o DINÁMICO según los parámetros
// recibidos en el JSON. Prepara las variables internas y envía comandos SCPI
// iniciales a la carga.
//
// PARÁMETROS:
// - funcion: "LIST" (lista de pasos) o "DYN" (dinámico alternancia)
// - res: JsonVariant con los datos de "Config.Resolution"
// - range: JsonVariant con los datos de "Config.Range"
//
// COMPORTAMIENTO INTERNO:
//
// --> MODO LIST:
//     Recibe: totalSteps, stepCurrent, stepDuration
//     Genera: Array softwareList con pasos escalados (1×current, 2×current, ...)
//     Ejemplo: 2 pasos con 5A cada uno = [5A, 10A]
//
// --> MODO DYN:
//     Recibe: corrienteA, tiempoA_seg, corrienteB, tiempoB_seg
//     Convierte tiempos de segundos a milisegundos
//     Configura alternancia entre dos valores
//
// RESPUESTA: No entrega respuesta JSON directa en esta función
// ============================================================================
void configLoad(String funcion, JsonVariant res, JsonVariant range) {
  // Comandos iniciales comunes a ambos modos
  sendSCPI("SYST:REM");  // Poner en modo remoto (SCPI)
  sendSCPI("INP 0");     // Desactivar entrada (carga OFF)
  sendSCPI("*CLS");      // Limpiar errores previos

  // Obtener rango de corriente desde Config.Range[1]
  // Si no viene dato o es 0, usar valor por defecto 30A
  float currentRange = range[1].as<float>();
  if (currentRange <= 0) currentRange = 30.0;

  if (funcion == "LIST") {
    // ====== CONFIGURACIÓN MODO LIST ======
    runDynamic = false;  // Desactivar modo dinámico si estaba activo

    // Extraer parámetros de Resolution
    totalSteps = res[0] | 0;            // Cantidad de pasos
    float stepCurrent = res[1] | 0.0;   // Corriente por paso
    float stepDuration = res[2] | 0.0;  // Duración de cada paso (segundos)

    // Generar lista escalada: paso 1 = 1×stepCurrent, paso 2 = 2×stepCurrent, etc.
    for (int i = 0; i < totalSteps && i < 30; i++) {
      softwareList[i].current = (i + 1) * stepCurrent;
      softwareList[i].duration = stepDuration;
    }

    // Configurar carga ITECH para modo corriente
    sendSCPI("FUNC CURRent");                           // Función: modo corriente
    sendSCPI("CURRent:RANGe " + String(currentRange));  // Rango máximo
    isListReady = true;                                 // Marcar que la lista está lista para ejecutar
  } else if (funcion == "DYN") {
    // ====== CONFIGURACIÓN MODO DINÁMICO ======
    isListReady = false;  // Desactivar modo lista si estaba activo

    // Extraer parámetros de Resolution: [corrA, tiempoA_seg, corrB, tiempoB_seg]
    dynA = res[0];                                     // Corriente estado A
    dynTimeA = (unsigned long)((float)res[1] * 1000);  // Tiempo A en ms
    dynB = res[2];                                     // Corriente estado B
    dynTimeB = (unsigned long)((float)res[3] * 1000);  // Tiempo B en ms

    // Configurar carga ITECH para modo corriente
    sendSCPI("FUNC CURRent");                           // Función: modo corriente
    sendSCPI("CURRent:RANGe " + String(currentRange));  // Rango máximo
    runDynamic = true;                                  // Activar modo dinámico
    dynState = false;                                   // Comenzar en estado A
    lastDynMillis = millis();                           // Capturar marca de tiempo inicial
  }
}



// ============================================================================
// FUNCIÓN: runSoftwareList()
// ============================================================================
// PROPÓSITO:
// Ejecuta secuencialmente todos los pasos de la lista previamente configurada.
// Aplica cada corriente y espera el tiempo especificado antes de pasar al siguiente.
//
// PROCESO:
// 1. Activar entrada (INP 1) → Carga ITECH comienza a conducir corriente
// 2. Para cada paso:
//    - Establecer corriente
//    - Esperar duración (en segundos)
// 3. Al finalizar: desactivar entrada (INP 0) → Carga OFF
// 4. Enviar JSON de confirmación al PC
//
// RESPUESTA:
// {"DEBUG":"LIST finalizado"}
//
// EJEMPLO DE EJECUCIÓN:
// Si la lista tiene 2 pasos: [5A/1seg, 10A/2seg]
// Resultado:
// - Activar carga
// - Aplicar 5A durante 1 segundo
// - Aplicar 10A durante 2 segundos
// - Desactivar carga
// - Enviar: {"DEBUG":"LIST finalizado"}
// ============================================================================
void runSoftwareList() {
  sendSCPI("INP 1");  // Activar carga (comienza a conducir corriente)

  // Ejecutar cada paso de la lista
  for (int i = 0; i < totalSteps; i++) {
    sendSCPI("CURRent " + String(softwareList[i].current, 3));  // Establecer corriente
    delay(softwareList[i].duration * 1000);                     // Esperar (convertir seg a ms)
  }

  isListReady = false;                                // Marcamos que la lista ya fue ejecutada
  sendSCPI("INP 0");                                  // Desactivar carga (carga OFF)
  Serial.println("{\"DEBUG\":\"LIST finalizado\"}");  // Confirmación al PC
}



void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas para JSON válido
  Serial.println("{\"debug\": \"" + str + "\"}");
}

// ============================================================================
// SETUP: Inicialización del sistema
// ============================================================================
// Se ejecuta una sola vez al encender o resetear la Pulsar C6
//
// INICIALIZACIONES:
// 1. Serial (USB) @9600 bps: Comunicación con PC
// 2. ITECH (UART1) @9600 bps en GPIO 4/5: Comunicación con carga
// 3. Envía JSON "System:Ready" para confirmar que está listo
// ============================================================================
void setup() {
  Serial.begin(9600);                       // Inicializar Serial USB para PC
  ITECH.begin(9600, SERIAL_8N1, RX1, TX1);  // Inicializar UART1 para carga
  delay(100);
  Serial.println("{\"System\":\"CV ITECH Ready\"}");  // Confirmación de inicialización
}

// ============================================================================
// LOOP: Bucle principal (se ejecuta continuamente)
// ============================================================================
// RESPONSABILIDADES:
// 1. Actualizar modo DINÁMICO: alterna entre corrienteA y corrienteB
// 2. Procesar comandos JSON recibidos del PC (Serial USB)
// 3. Ejecutar acciones según el comando: activar, desactivar, medir, etc.
//
// FLUJO:
// ┌─────────────────────────────────────────────────────────────┐
// │ 1. Si modo DYN activo → Actualizar alternancia de corriente │
// │ 2. Si hay JSON en Serial → Procesar comando                  │
// │    - Extraer: Funcion, Config, Start, Range                 │
// │    - Ejecutar acción correspondiente                          │
// │    - Enviar respuesta JSON al PC                             │
// └─────────────────────────────────────────────────────────────┘
// ============================================================================
void loop() {
  // ========== ACTUALIZACIÓN MODO DINÁMICO ==========
  // Si el modo dinámico está activo, alternar entre dos corrientes
  if (runDynamic) {
    unsigned long currentMillis = millis();
    // Determinar cuál es el intervalo actual (A o B)
    unsigned long interval = dynState ? dynTimeB : dynTimeA;

    // Si ha pasado el tiempo del intervalo actual: cambiar estado
    if (currentMillis - lastDynMillis >= interval) {
      lastDynMillis = currentMillis;          // Actualizar marca de tiempo
      dynState = !dynState;                   // Alternar estado (A ↔ B)
      float val = dynState ? dynB : dynA;     // Obtener valor de corriente
      sendSCPI("CURRent " + String(val, 3));  // Enviar a carga
    }
  }

  // ========== PROCESAMIENTO DE COMANDOS JSON ==========
  // Si la PC envía datos por Serial USB
  if (Serial.available()) {
    DynamicJsonDocument doc(1024);  // Buffer 1024 bytes para JSON
    DeserializationError error = deserializeJson(doc, Serial);

    // Si el JSON se parsea correctamente
    if (!error) {
      String funcion = doc["Funcion"] | "";  // Extraer "Funcion"
      String start = doc["Start"] | "";      // Extraer "Start"

      if (funcion == "ping") {
        serialDebug("ping - pong");
      }

      // ===== COMANDOS: Configurar LIST o DYN =====
      if (funcion == "LIST" || funcion == "DYN") {
        configLoad(funcion, doc["Config"]["Resolution"], doc["Config"]["Range"]);
        Serial.print("{\"CONF\":\"");
        Serial.print(funcion);
        Serial.println("\",\"response\":true}");

        // Si Start es "CFG_ON": ejecutar inmediatamente
        /*
        if (start == "CFG_ON") {
          if (funcion == "LIST") runSoftwareList();
          else if (funcion == "DYN") sendSCPI("INP 1"); // Activar carga
        }
        */
      }
      // ===== COMANDO: Leer mediciones =====
      else if (start == "Read") {
        takeMeasurements();  // Obtener V, I, P y enviar al PC
      }
      // ===== COMANDO: Activar carga =====
      else if (start == "CFG_ON") {
        if (isListReady) runSoftwareList();  // Si hay lista: ejecutar
        else sendSCPI("INP 1");              // Si no: solo activar
        Serial.println("{\"CONF\":\"CFG_ON\",\"response\":true}");
      }
      // ===== COMANDO: Desactivar carga =====
      else if (start == "CFG_OFF") {
        sendSCPI("INP 0");    // Desactivar carga
        runDynamic = false;   // Detener modo dinámico si estaba activo
        isListReady = false;  // Limpiar bandera de lista
        Serial.println("{\"CONF\":\"CFG_OFF\",\"response\":true}");
      }
    }
  }
}