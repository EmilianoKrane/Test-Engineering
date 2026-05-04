#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//////////////////// Configuración general ////////////////////
#define SDA_PIN 6
#define SCL_PIN 7

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// RTC
RTC_DS3231 rtc;

// Español
const char* semana[7] = { "Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado" };
const char* monthsNames[12] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio",
                                "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre" };

// Estado de alarma
bool alarmaEnabled = true;
int alarmHour = 16;
int alarmMinute = 21;
int lastTriggerMinute = -1;

// Parpadeo de los dos puntos
bool colonOn = true;
unsigned long lastBlink = 0;

// Control de impresión por Serial
unsigned long lastPrint = 0;

//////////////////// Utilidades ////////////////////
void printDateTimeSerial(const DateTime& dt) {
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
}

bool parseDateTime(const String& s, DateTime& out) {
  // Formato: YYYY-MM-DD HH:MM:SS
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
  // Formato: HH:MM (24h)
  if (s.length() < 5) return false;
  h = s.substring(0, 2).toInt();
  m = s.substring(3, 5).toInt();
  if (h < 0 || h > 23 || m < 0 || m > 59) return false;
  return true;
}

void showMenuHelp() {
  Serial.println(F("\n=== MENU RTC/ALARMA ==="));
  Serial.println(F("Comandos:"));
  Serial.println(F("  H=YYYY-MM-DD HH:MM:SS   -> Ajustar fecha/hora"));
  Serial.println(F("  A=HH:MM                  -> Ajustar hora de alarma (24h)"));
  Serial.println(F("  ENA                      -> Habilitar alarma"));
  Serial.println(F("  DIS                      -> Deshabilitar alarma"));
  Serial.println(F("  NOW?                     -> Mostrar fecha/hora actual"));
  Serial.println(F("  STATUS                   -> Mostrar configuracion"));
  Serial.println(F("  HELP                     -> Mostrar esta ayuda\n"));
}

void showStatus() {
  Serial.println(F("\n--- ESTADO ---"));
  Serial.print(F("Alarma: "));
  Serial.println(alarmaEnabled ? F("HABILITADA") : F("DESHABILITADA"));
  Serial.print(F("Hora de alarma: "));
  if (alarmHour < 10) Serial.print('0');
  Serial.print(alarmHour);
  Serial.print(':');
  if (alarmMinute < 10) Serial.print('0');
  Serial.println(alarmMinute);
  Serial.print(F("Hora actual: "));
  DateTime now = rtc.now();
  printDateTimeSerial(now);
  Serial.println();
  Serial.println(F("---------------\n"));
}

void handleSerialMenu() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  // Conserva una copia para parsear números (no dependas de mayúsculas)
  String raw = line;

  String upper = line;
  upper.toUpperCase();

  if (upper.startsWith("H=")) {
    String ts = raw.substring(2);
    ts.trim();
    DateTime dt;
    if (parseDateTime(ts, dt)) {
      rtc.adjust(dt);
      Serial.print(F("Fecha/hora ajustada a: "));
      printDateTimeSerial(dt);
      Serial.println();
    } else {
      Serial.println(F("Formato invalido. Usa: H=YYYY-MM-DD HH:MM:SS"));
    }
  } else if (upper.startsWith("A=")) {
    String hhmm = raw.substring(2);
    hhmm.trim();
    int h, m;
    if (parseAlarm(hhmm, h, m)) {
      alarmHour = h;
      alarmMinute = m;
      lastTriggerMinute = -1;
      Serial.print(F("Alarma ajustada a "));
      if (alarmHour < 10) Serial.print('0');
      Serial.print(alarmHour);
      Serial.print(':');
      if (alarmMinute < 10) Serial.print('0');
      Serial.println(alarmMinute);
    } else {
      Serial.println(F("Formato invalido. Usa: A=HH:MM (24h)"));
    }
  } else if (upper == "ENA") {
    alarmaEnabled = true;
    Serial.println(F("Alarma habilitada."));
  } else if (upper == "DIS") {
    alarmaEnabled = false;
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println(F("Alarma deshabilitada."));
  } else if (upper == "NOW?") {
    DateTime now = rtc.now();
    printDateTimeSerial(now);
    Serial.println();
  } else if (upper == "STATUS") {
    showStatus();
  } else if (upper == "HELP") {
    showMenuHelp();
  } else {
    Serial.println(F("Comando no reconocido. Escribe HELP para ver opciones."));
  }
}

void checkAlarm(const DateTime& now) {
  if (!alarmaEnabled) return;

  if (now.hour() == alarmHour && now.minute() == alarmMinute && now.second() == 0) {
    if (lastTriggerMinute != now.minute()) {
      Serial.println(F("** ALARMA **"));
      digitalWrite(LED_BUILTIN, HIGH);
      lastTriggerMinute = now.minute();
    }
  } else {
    if (now.minute() != alarmMinute) {
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}

//////////////////// Dibujo en OLED ////////////////////
void drawBellIcon(int16_t x, int16_t y) {
  // Campanita simple (8x8 aprox)
  display.drawTriangle(x+2, y+6, x+5, y, x+8, y+6, SSD1306_WHITE); // cono
  display.drawLine(x+1, y+6, x+9, y+6, SSD1306_WHITE);            // borde inferior
  display.drawLine(x+3, y+7, x+7, y+7, SSD1306_WHITE);            // badajo/soporte
  display.drawPixel(x+5, y+8, SSD1306_WHITE);                     // bolita
}

void renderClock(const DateTime& now) {
  display.clearDisplay();

  // 1) Encabezado: día de la semana
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); // 6x8 px por char
  display.setCursor(0, 0);
  display.print(semana[now.dayOfTheWeek()]);

  // Indicador de alarma a la derecha
  if (alarmaEnabled) {
    drawBellIcon(SCREEN_WIDTH - 12, 0);
  }

  // 2) Hora grande (HH:MM) centrada
  // Usamos TextSize=3 -> cada char ~ 18 px ancho (6*3) aprox; dejamos 5 chars ("HH:MM")
  // Calculamos X para centrar
  display.setTextSize(3);
  int charW = 6 * 3;   // ancho aprox por char (fuente base 5+1)
  int textW = charW * 5; // "HH:MM"
  int x = (SCREEN_WIDTH - textW) / 2;
  int y = 18;

  // Parpadeo de dos puntos
  bool showColon = colonOn;

  // Horas
  display.setCursor(x, y);
  if (now.hour() < 10) display.print('0');
  display.print(now.hour());

  // Dos puntos
  display.setCursor(x + charW*2, y);
  display.print(showColon ? ":" : " ");

  // Minutos
  display.setCursor(x + charW*3, y);
  if (now.minute() < 10) display.print('0');
  display.print(now.minute());

  // 3) Fecha abajo
  display.setTextSize(1);
  display.setCursor(0, 54);
  display.print(now.day());
  display.print(" ");
  display.print(monthsNames[now.month()-1]);
  display.print(" ");
  display.print(now.year());

  display.display();
}

//////////////////// Setup & Loop ////////////////////
void setup() {
#if defined(ARDUINO_ARCH_ESP32)
  Wire.begin(SDA_PIN, SCL_PIN);
#else
  Wire.begin();
#endif

  Serial.begin(115200);
  delay(200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("No se encontro OLED SSD1306. Verifica cableado y direccion (0x3C)."));
    while (1) delay(10);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Reloj OLED + RTC"));
  display.display();
  delay(700);

  if (!rtc.begin()) {
    Serial.println(F("No se encontro el RTC DS3231. Verifica cableado/alimentacion."));
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println(F("RTC sin hora valida. Ajustando a hora de compilacion..."));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  showMenuHelp();
  showStatus();
}

void loop() {
  handleSerialMenu();

  DateTime now = rtc.now();
  checkAlarm(now);

  // Parpadeo de los dos puntos cada ~500 ms
  if (millis() - lastBlink > 500) {
    colonOn = !colonOn;
    lastBlink = millis();
  }

  // Redibuja siempre (barato en 128x64)
  renderClock(now);

  // Mensaje por Serial cada ~2s (opcional)
  if (millis() - lastPrint > 2000) {
    Serial.print(F("[RTC] "));
    printDateTimeSerial(now);
    Serial.println();
    lastPrint = millis();
  }

  delay(10);
}