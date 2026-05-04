/*
Firmware de prueba para el módulo UE0108 Test DevLab: I2C DS3231 RTC Module.

Conexión: el módulo RTC se debe conectar mediante conector Qwiic/JST al conector JST
de una Pulsar C6. La comunicación I2C utiliza los GPIOs 6 (SDA) y 7 (SCL).

Funcionalidad:
- Inicializa el RTC DS3231.
- Verifica la hora actual.
- Ajusta la hora mediante comandos JSON recibidos por UART2.
- Responde con JSON a la interfaz de pruebas UNIT.

Nota: las instrucciones JSON llegan a través del CH340 conectado al UART2
con GPIOs 15 (RX2) y 19 (TX2). Si se desea usar el UART nativo de la Pulsar,
se deberá adaptar el código para utilizar UART1 en lugar del objeto PagWeb.
*/



#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "RTClib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==== DECLARACIÓN DE PINES ====
#define RUN_BUTTON 4  // >> GPIO04 Arranque por Botonera
#define SDA_PIN 6     // >> GPIO06 SDA de I2C
#define SCL_PIN 7     // >> GPIO07 SCL de I2C
#define RX2 15        // >> GPIO15 como RX de UART2
#define TX2 19        // >> GPIO19 como TX de UART2

// ==== CREACIÓN DE OBJETOS ====
HardwareSerial PagWeb(1);  // Objeto UART2 para comunicación con la interfaz PagWeb / UNIT.
RTC_DS3231 rtc;            // Objeto para controlar el RTC DS3231.

String JSON_entrada;                   ///< Buffer para recibir líneas JSON desde PagWeb.
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parseo de datos entrantes.

String JSON_salida;                 ///< Buffer opcional para respuestas JSON (no usado directamente).
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para construir respuestas.

// ==== VARIABLES DE MANEJO DE RTC ====
const char* semana[7] = { "Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado" };
const char* monthsNames[12] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio",
                                "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre" };

bool alarmaEnabled = true;  // Estado de alarma
int alarmHour = 16;
int alarmMinute = 21;
int lastTriggerMinute = -1;

bool colonOn = true;  // Parpadeo de los dos puntos
unsigned long lastBlink = 0;

// ==== FUNCIONES DE UTILIDAD ====
bool parseDateTime(const String& s, DateTime& out) {
  // Convierte un string con formato ISO "YYYY-MM-DD HH:MM:SS" en un objeto DateTime.
  if (s.length() < 19) return false;
  int Y = s.substring(0, 4).toInt();
  int M = s.substring(5, 7).toInt();
  int D = s.substring(8, 10).toInt();
  int h = s.substring(11, 13).toInt();
  int m = s.substring(14, 16).toInt();
  int sec = s.substring(17, 19).toInt();
  if (Y < 2000 || M < 1 || M > 12 || D < 1 || D > 31 || h < 0 || h > 23 || m < 0 || m > 59 || sec < 0 || sec > 59) {
    return false;
  }
  out = DateTime(Y, M, D, h, m, sec);
  return true;
}

bool parseAlarm(const String& s, int& h, int& m) {
  // Convierte un string "HH:MM" en hora y minuto. Función reservada para uso futuro.
  if (s.length() < 5) return false;
  h = s.substring(0, 2).toInt();
  m = s.substring(3, 5).toInt();
  if (h < 0 || h > 23 || m < 0 || m > 59) return false;
  return true;
}

void checkAlarm(const DateTime& now) {
  if (!alarmaEnabled) return;

  if (now.hour() == alarmHour && now.minute() == alarmMinute && now.second() == 0) {
    if (lastTriggerMinute != now.minute()) {
      Serial.println(F("** ALARMA **"));
      //digitalWrite(LED_BUILTIN, HIGH);
      lastTriggerMinute = now.minute();
    }
  } else {
    if (now.minute() != alarmMinute) {
      //digitalWrite(LED_BUILTIN, LOW);
    }
  }
}

void DateTimeJSON(const DateTime& dt) {
  // Construye un JSON con la fecha/hora actual y lo envía por UART2 (PagWeb).
  sendJSON.clear();
  bool hourSerial = true;
  if (hourSerial) {
    Serial.print(semana[dt.dayOfTheWeek()]);
    Serial.print(" ");
    Serial.print(dt.day());
    Serial.print(" de ");
    Serial.print(monthsNames[dt.month() - 1]);
    Serial.print(" de ");
    Serial.print(dt.year());
    Serial.print("  ");
    if (dt.hour() < 10) Serial.print('0');
    Serial.print(dt.hour());
    Serial.print(':');
    if (dt.minute() < 10) Serial.print('0');
    Serial.print(dt.minute());
    Serial.print(':');
    if (dt.second() < 10) Serial.print('0');
    Serial.print(dt.second());
    Serial.println();
  }

  // ==== Envío de datos por JSON ====
  sendJSON["hour"] = dt.hour();
  sendJSON["min"] = dt.minute();
  sendJSON["seg"] = dt.second();
  sendJSON["day"] = dt.day();
  sendJSON["month"] = monthsNames[dt.month() - 1];
  sendJSON["year"] = dt.year();

  serializeJson(sendJSON, PagWeb);
  PagWeb.println();
}


// ==== FUNCIONES DE FLUJO ====
void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  Serial.println("{\"debug\": \"" + str + "\"}");
}

void pagwebDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  PagWeb.println("{\"debug\": \"" + str + "\"}");
}

void setup() {

  // ==== Inicialización de comunicación serial ====
  Serial.begin(115200);                            // Puerto USB para debug local.
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);      // UART2 para comandos JSON UNIT.
  delay(100);
  serialDebug("Serial initialized...");
  pagwebDebug("Test initialized...");

  Wire.begin(SDA_PIN, SCL_PIN);  // Inicializa el bus I2C con los pines SDA/SCL definidos.

  // ==== Declaración de pines ====
  pinMode(RUN_BUTTON, INPUT);  // Configura el botón de arranque en modo entrada.
}


void loop() {

  // ==== Lectura del botón de arranque ====
  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);
    sendJSON.clear();  // Limpia cualquier dato previo.

    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Arranque por botonera");
      sendJSON["Run"] = "OK";           // Envío de confirmación JSON de arranque.
      serializeJson(sendJSON, PagWeb);   // Envío de datos por JSON a la interfaz PagWeb.
      PagWeb.println();
    }
  }


  // ==== Recepción de comandos JSON por PagWeb / UART2 ====
  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // ---- Valores recibidos por el JSON de Entrada ----
    String Function = receiveJSON["Function"];

    int opc = 0;
    if (Function == "ping") opc = 1;            // {"Function":"ping"}
    else if (Function == "init") opc = 2;       // {"Function":"init"}
    else if (Function == "checkHour") opc = 3;  // {"Function":"checkHour"}
    else if (Function == "setHour") opc = 4;    // {"Function":"setHour", "datetime":"2026-05-04 15:18:30"}

    switch (opc) {
      case 1:
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 2:
        {
          sendJSON.clear();
          if (rtc.begin()) sendJSON["Result"] = "OK";
          else sendJSON["debug"] = "RTC DS3231 no detected...";

          if (rtc.lostPower()) {
            serialDebug("RTC sin hora valida. Ajustando a hora de compilacion...");
            pagwebDebug("RTC sin hora valida. Ajustando a hora de compilacion...");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
          }

          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 3:
        {
          sendJSON.clear();
          Serial.print(F("Hora actual: "));
          for (int i = 0; i < 10; i++) {
            DateTime now = rtc.now();
            DateTimeJSON(now);
            delay(1000);
          }
          break;
        }

      case 4:
        {
          sendJSON.clear();
          DateTime nuevaHora;
          String dtStr = receiveJSON["datetime"];  // Espera un string como "2026-05-04 15:15:00"

          if (parseDateTime(dtStr, nuevaHora)) {
            rtc.adjust(nuevaHora);
            sendJSON["Result"] = "OK";
            serialDebug("Hora ajustada manualmente vía Web");
          } else {
            sendJSON["Result"] = "Error de formato";
          }

          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      default: break;
    }
  }
}
