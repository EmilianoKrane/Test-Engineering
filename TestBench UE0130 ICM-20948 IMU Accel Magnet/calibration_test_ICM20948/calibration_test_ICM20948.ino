/**
 * @file test.ino
 * @brief Firmware de caracterización y validación para el acelerómetro ICM-20948.
 * @details Este programa realiza un barrido automático (sweep) de configuraciones en el acelerómetro,
 * cruzando 4 escalas de medición (Full Scale: +-2G, +-4G, +-8G, +-16G) con 8 configuraciones de 
 * filtro pasa bajas digital (DLPF) y tasas de muestreo (ODR). 
 * * Su propósito es permitir al desarrollador observar empíricamente la estabilidad y el ruido del 
 * sensor (en los ejes X, Y, Z) bajo diferentes parámetros. Se asume que el sensor está estático 
 * y plano sobre una mesa (donde Z debería leer ~1G y X/Y ~0G). Las configuraciones que exceden 
 * los límites de la librería (ej. ODR=1125Hz con divisores incompatibles) reportarán un fallo 
 * controlado por diseño.
 */

#include <Wire.h>
#include <DevLab_ICM20948.h>

// -----------------------------------------------------------
// PINES I2C — ESP32C6 NANO Unit Electronics
// -----------------------------------------------------------
/** @brief I2C SDA pin used by the example board. */
#define SDA_PIN 6
/** @brief I2C SCL pin used by the example board. */
#define SCL_PIN 7
/** @brief I2C bus speed in hertz. */
#define I2C_FREQ 400000UL  // Fast Mode 400 kHz

// -----------------------------------------------------------
// DIRECCIONES I2C
// -----------------------------------------------------------
/** @brief ICM-20948 I2C address selected by the AD0 pin. */
#define ICM_ADDR 0x69  // AD0 = LOW (GND)

/** @brief Accelerometer DLPF divider and timing configuration. */
struct configDLPF {
  /** @brief Index into the accelDLPF lookup table. */
  uint8_t dlpf_idx;
  /** @brief Sample-rate divider written to the IMU register. */
  uint16_t div;  // el valor REAL del registro, 0-4095, sin ambigüedad
  /** @brief Noise bandwidth in hertz for reporting. */
  float nbw_hz;
  /** @brief Output data rate in hertz for timing calculations. */
  float odr_hz;  // = 1125.0f / (1 + div), para calcular tiempos
  /** @brief Text label printed with each captured sample. */
  const char* nombre;
};

/** @brief Accelerometer DLPF configurations exercised by this validation. */
const configDLPF configsDLPF[] = {
  // idx  div   NBW(Hz)   ODR(Hz)    nombre
  { 0, 0, 265.0f, 1125.0f, "DLPF0_246 | NBW=265Hz | ODR=1125Hz (4.2x)" },
  { 1, 0, 265.0f, 1125.0f, "DLPF_246  | NBW=265Hz | ODR=1125Hz (4.2x)" },
  { 2, 1, 136.0f, 562.5f, "DLPF_111  | NBW=136Hz | ODR= 562Hz (4.1x)" },
  { 3, 3, 68.8f, 281.3f, "DLPF_50   | NBW= 69Hz | ODR= 281Hz (4.1x)" },
  { 4, 7, 34.4f, 140.6f, "DLPF_24   | NBW= 34Hz | ODR= 141Hz (4.1x)" },
  { 5, 15, 17.0f, 70.3f, "DLPF_12   | NBW= 17Hz | ODR=  70Hz (4.1x)" },
  { 6, 31, 8.3f, 35.2f, "DLPF_6    | NBW=  8Hz | ODR=  35Hz (4.2x)" },
  { 7, 0, 499.0f, 1125.0f, "DLPF_473  | NBW=499Hz | ODR=1125Hz (2.3x)" },
};

//------------------------------------------------------
//  ARRAYS DE CONFIGURACIÓN
// -----------------------------------------------------------
/** @brief Number of accelerometer DLPF configurations. */
const uint8_t N_DLPF = sizeof(configsDLPF) / sizeof(configsDLPF[0]);

/** @brief Accelerometer full-scale ranges available for validation. */
const ICM20948_Accel_FullScale accelCfg[] = { ICM20948_Accel_FullScale::G_2, ICM20948_Accel_FullScale::G_4, ICM20948_Accel_FullScale::G_8, ICM20948_Accel_FullScale::G_16 };
/** @brief Text labels matching accelCfg. */
const char* etiquetasAccelCfg[] = { "+-2G", "+-4G", "+-8G", "+-16G" };
/** @brief Number of accelerometer full-scale ranges. */
const uint8_t N_FS = 4;

/** @brief Accelerometer DLPF enum values matching configsDLPF. */
const ICM20948_Accel_DLPFCFG accelDLPF[] = { ICM20948_Accel_DLPFCFG::DLPF0_246HZ, ICM20948_Accel_DLPFCFG::DLPF_246HZ, ICM20948_Accel_DLPFCFG::DLPF_111HZ, ICM20948_Accel_DLPFCFG::DLPF_50HZ, ICM20948_Accel_DLPFCFG::DLPF_24HZ, ICM20948_Accel_DLPFCFG::DLPF_12HZ, ICM20948_Accel_DLPFCFG::DLPF_6HZ, ICM20948_Accel_DLPFCFG::DLPF_473HZ };
/** @brief Text labels describing accelDLPF bandwidth pairs. */
const char* etiquetasAccelDLPFCFG[] = { "246/265", "246/265", "111/136", "50.4/68.8", "23.9/34.4", "11.5/17.0", "5.7/8.3", "473/499" };

/** @brief Accelerometer averaging settings available for validation. */
const ICM20948_Accel_Average accelAVG[] = { ICM20948_Accel_Average::AVG_1_OR_4, ICM20948_Accel_Average::AVG_8, ICM20948_Accel_Average::AVG_16, ICM20948_Accel_Average::AVG_32 };
/** @brief Text labels matching accelAVG. */
const char* etiquetasAccelAVG[] = { "1 OR 4", "8", "16", "32" };

/** @brief Raw accelerometer sample-rate divider values for experiments. */
const uint16_t accelSR[] = { 0, 1, 3, 5, 7, 10, 15, 22, 31, 63, 127, 255, 513, 1022, 2044, 4095 };
/** @brief Text labels matching accelSR output data rates. */
const char* etiquetasSR[] = { "1125.0", "562.5", "281.3", "187.5", "140.6", "102.3", "70.3", "48.9", "35.2", "17.6", "8.8", "4.4", "2.2", "1.1", "0.55", "0.27" };

// -----------------------------------------------------------
//  INSTANCIA DEL SENSOR
// -----------------------------------------------------------
/** @brief Global ICM-20948 driver instance. */
DevLab_ICM20948 imu;

/** @brief Count of validation passes reserved for future checks. */
uint8_t total_pass = 0;
/** @brief Count of validation failures reserved for future checks. */
uint8_t total_fail = 0;
/** @brief Count of validation warnings reserved for future checks. */
uint8_t total_warn = 0;

// ============================================================
//  UTILIDADES DE IMPRESIÓN
// ============================================================

/** @brief Print a separator line to Serial. */
void printLinea() {
  Serial.println(F("------------------------------------------------------------"));
}

/** @brief Print a double separator line to Serial. */
void printDobleLinea() {
  Serial.println(F("============================================================"));
}

/**
 * @brief Apply one accelerometer full-scale and DLPF configuration.
 *
 * @param fs_idx Index into accelCfg.
 * @param cfg DLPF timing and divider configuration.
 * @return true if all configuration writes succeeded, otherwise false.
 */
bool applySettings(uint8_t fs_idx, const configDLPF& cfg) {
  uint8_t dlpf_val = (uint8_t)accelDLPF[cfg.dlpf_idx];
  bool ok = true;

  ok &= imu.setAccelScale(accelCfg[fs_idx]);
  ok &= imu.setAccelDLPF(dlpf_val, false);
  ok &= imu.setAccelDivRate(cfg.div);

  return ok;
}


/**
 * @brief Verify the IMU identity register.
 */
void firstStage() {
  uint8_t who;
  if (!imu.readWhoAmI(who) || who != 0xEA) {
    Serial.println(F("ERROR: WHO_AM_I mismatch"));
    while (1) delay(200);
  }
  Serial.print(F("WHO_AM_I = 0x"));
  Serial.println(who, HEX);
}

/**
 * @brief Ejecuta el barrido de pruebas del acelerómetro y muestra los resultados por Serial.
 * @details Esta función automatiza la caracterización mediante dos bucles anidados:
 * 1. Bucle externo: Itera sobre las 4 escalas físicas del sensor (FS: +-2G, 4G, 8G, 16G).
 * 2. Bucle interno: Itera sobre las 8 configuraciones de filtro (DLPF) y frecuencia (ODR).
 * * Por cada combinación de (Escala + Filtro), el firmware escribe en los registros I2C.
 * Si la escritura es exitosa, realiza una pausa dinámica para que el filtro interno del 
 * hardware se estabilice (settling time), y luego toma 3 lecturas consecutivas espaciadas
 * según el periodo calculado del ODR.
 */
void runSweep() {
  printDobleLinea();
  Serial.println(F("Acelerometro ICM-200948 | UNIT Electronics"));
  Serial.printf("%d DLPFS x %d FS = %d combinaciones\n", N_DLPF, N_FS, N_DLPF * N_FS);
  Serial.println(F("Coloque el sensor plano y quieto mirando hacia arriba"));
  printDobleLinea();

  // 1. Bucle externo: Recorre las 4 Escalas Completas (FS)
  for (uint8_t fs = 0; fs < N_FS; fs++) {

    // 2. Bucle interno: Recorre los 8 Filtros (DLPF)
    for (uint8_t d = 0; d < N_DLPF; d++) {

      Serial.printf(" ESCALA: %s | BLOQUE %d/%d: %s\n", etiquetasAccelCfg[fs], d + 1, N_DLPF, configsDLPF[d].nombre);
      Serial.printf(" ODR = %.1fHz | Delay = %lums | Periodo= %.1fms\n",
                    configsDLPF[d].odr_hz,
                    max((uint32_t)(10000.0f / configsDLPF[d].odr_hz), (uint32_t)50),
                    1000.0f / configsDLPF[d].odr_hz);
      printDobleLinea();

      // Aplica los parámetros de Escala y Filtro al sensor
      bool cfg_ok = applySettings(fs, configsDLPF[d]);

      if (!cfg_ok) {
        Serial.println(F("  ERROR: applySettings() fallo"));
        printLinea();
        continue;  // Si la configuración falla (ej. ODR no soportado), salta a la siguiente
      }

      // Esperar que el filtro estabilice (10 periodos del ODR, min 50ms)
      uint32_t settle_ms = max((uint32_t)(10000.0f / configsDLPF[d].odr_hz), (uint32_t)50);
      delay(settle_ms);

      // Tomar 3 lecturas espaciadas por el periodo exacto del ODR
      uint32_t periodo_ms = max((uint32_t)(1000.0f / configsDLPF[d].odr_hz), (uint32_t)1);
      float ax, ay, az;

      for (uint8_t r = 0; r < 3; r++) {
        delay(periodo_ms);
        if (imu.readAccel(ax, ay, az)) {
          Serial.printf("  [%d] Ax=%+.3f  Ay=%+.3f  Az=%+.3f g\n", r + 1, ax, ay, az);
        } else {
          Serial.println(F("  [?] readAccel() fallo"));
        }
      }
      printLinea();
    }
  }
}

/**
 * @brief Initialize I2C, reset/wake the IMU, enable sensors, and start sweep.
 */
void setup() {

  Serial.begin(115200);
  delay(200);
  Serial.println(F("ICM-20948 — I2C Basic"));
  printDobleLinea();
  Serial.println(F("  ICM-20948 VALIDACION ACELEROMETRO"));
  printDobleLinea();
  // I2C init (UNO/ESP32 defaults). For ESP32 custom pins: Wire.begin(SDA,SCL);
  Wire.begin(SDA_PIN, SCL_PIN);
  // Attach IMU
  if (!imu.beginI2C(ICM_ADDR, Wire, I2C_FREQ)) {
    Serial.println(F("ERROR: begin(I2C) failed"));
    while (1) delay(200);
  }

  firstStage();

  imu.softReset();

  imu.sleep(false);
  delay(50);
  /* Enable all sensors. */
  if (!imu.setSensors(true, true, true)) {
    Serial.println(F("ERROR: setSensors failed"));
  }
  Serial.println(F("  Sensor listo.\n"));
  Serial.println(F("  Sensor PLANO y QUIETO sobre la mesa."));
  Serial.println(F("  Iniciando en 5 segundos..."));
  for (int i = 5; i > 0; i--) {
    Serial.printf("  %d...\n", i);
    delay(1000);
  }
  runSweep();
}

/**
 * @brief Optional post-sweep loop for manual accelerometer reads.
 */
void loop() {
  /*float ax, ay, az;
  if (imu.readAccel(ax, ay, az)) {
    Serial.printf("X:%.3f  Y:%.3f  Z:%.3f\n", ax, ay, az);
  }
  delay(200);*/
}