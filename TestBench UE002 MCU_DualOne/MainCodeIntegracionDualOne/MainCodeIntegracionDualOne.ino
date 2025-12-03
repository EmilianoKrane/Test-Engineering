/* 
===== CÓDIGO UNIT DUAL ONE JSON Integración RP2040 ====
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include "DHT.h"


// ==== Declaración de pines ====
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define SCL_PIN 5         // SCL pin D2
#define SDA_PIN 4         // SDA pin D3

#define DHT_PIN 9      // Pin DHT D4
#define DHTTYPE DHT11  // Tipo de sensor

#define BUZZER_PIN 11  // GPIO11 D5 para Buzzer en Shield
#define D7_PIN 10      // GPIO10 D7 Pin Salida Dig
#define D8_PIN 2       // GPIO2  D8 Pin Entrada Dig
#define RGB_RD09 3     // GPIO3  Rojo RGB Shield
#define RGB_GD10 17    // GPIO17 Verde RGB Shield
#define RGB_BD11 19    // GPIO19 Azul RGB Shield

#define LED_NEOP 24  // PIN DEDICADO AL NEOPIXEL WS2812
#define LED_BUIL 25  // Led Builtin GPIO25

#define TEMT6_PIN 29  // A0 para lectura de sensor de luz


// ==== Creación de objetos ====
DHT dht(DHT_PIN, DHTTYPE);                                                    // Objeto Sensor DHT Hum Y Temp
Adafruit_NeoPixel np = Adafruit_NeoPixel(1, LED_NEOP, NEO_GRB + NEO_KHZ800);  // Objeto NeoPixel
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);     // Objeto de la OLED

// ==== Declaración de variables
String JSON_entrada;
StaticJsonDocument<200> sendJSON;
StaticJsonDocument<200> receiveJSON;

void setup() {

  Serial.begin(115200);
  Serial.println("UART0 listo para comunicación...");
  Serial1.begin(115200);

  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();  // Inicialización de I2C

  analogReadResolution(12);  // Inicialización Resolución del ADC
  np.begin();                // Inicialización de objeto NeoPixel
  dht.begin();               // Inicialización del DHT

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();

  // Set text size and color
  display.setTextSize(1.5);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 0);
  display.println(F("Test de Prueba"));
  display.setCursor(30, 10);
  display.println(F("MCU DualOne"));
  display.display();  // Show initial text

  // ==== Declaración de GPIOs
  // ==== Salidas
  pinMode(LED_BUIL, OUTPUT);    // Led Builtin GPIO25
  pinMode(BUZZER_PIN, OUTPUT);  // BUZZER D5
  pinMode(D7_PIN, OUTPUT);      // Salida Digital D7

  // ==== Entradas
  pinMode(D8_PIN, INPUT);     // Entrada Digital D8
  pinMode(TEMT6_PIN, INPUT);  // A3 para sensor TEMT6000
}

void loop() {

  if (Serial1.available()) {  // Rutina de chequeo de GPIOs

    JSON_entrada = Serial1.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "meas") opc = 1;

      switch (opc) {
        case 1:

          // ==== Constructor OLED ====
          display.clearDisplay();
          display.setCursor(0, 10);
          display.setTextSize(1);
          display.print(F("Rutina: "));
          display.println("Check GPIOs");
          display.display();
          // ==== Constructor OLED ====

          // Validación D0 y D1

          // Validación D2 y D3 OLED

          // Validación D4 Sensor Hum Y Temp en Shield
          String read = DHTmeas();
          writeLCD(read, 0);

          // Validación D5 Buzzer en Shield
          melodyBuzzer();

          // Validación D6 IR Receptor en Shield

          // Validación D7 y D8 en Shield
          String statusDig = sequenceDIG();
          writeLCD("D7 y D8: " + statusDig, 0);

          // Validación D9 - D11 RGB LED Shield

          // Validación A3 Sensor de Luz TEMT6000
          float med = lightSensor();
          writeLCD("Luz: ", med);


          break;
      }
    }
  } else {
    // Rutina de demo
    sequenceNeop();
  }
}




// ==== DECLARACIÓN DE FUNCIONES AUXILIARES ====

void sequenceNeop() {
  np.setBrightness(20);

  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(255, 0, 0));
  np.show();
  delay(500);
  digitalWrite(LED_BUIL, LOW);
  digitalWrite(RGB_RD09, HIGH);
  digitalWrite(RGB_GD10, LOW);
  digitalWrite(RGB_BD11, LOW);

    delay(500);

  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(0, 255, 0));
  np.show();
  delay(500);
  digitalWrite(LED_BUIL, LOW);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, HIGH);
  digitalWrite(RGB_BD11, LOW);

    delay(500);

  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(0, 0, 255));
  np.show();
  delay(500);
  digitalWrite(LED_BUIL, LOW);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, LOW);
  digitalWrite(RGB_BD11, HIGH);

    delay(500);

  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(0, 0, 0));
  np.show();
  delay(500);
  digitalWrite(LED_BUIL, LOW);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, LOW);
  digitalWrite(RGB_BD11, LOW);

    delay(500);
}


float lightSensor() {
  float lect = analogRead(TEMT6_PIN);
  Serial.println("El valor es: " + String(lect));
  delay(50);

  return lect;
}

void writeLCD(String label, float med) {

  if (med != 0) {
    display.print(label);
    display.println(med);
  } else {
    display.println(label);
  }

  display.display();
}

String DHTmeas() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();  // °C

  // Validación: Si falla la lectura
  if (isnan(h) || isnan(t)) {
    Serial.println("Error leyendo el DHT11");
    return "Temp:Fail | Hum:Fail";
  }

  Serial.print("Temperatura: ");
  Serial.print(t);
  Serial.println(" °C\t");

  Serial.print("Humedad: ");
  Serial.print(h);
  Serial.print(" %");

  return "Temp:" + String(t) + "|Hum:" + String(h);
}



String sequenceDIG() {

  byte testSequence = 0b10101100;

  for (int i = 7; i >= 0; i--) {
    // Obtener el bit i
    int bitToSend = (testSequence >> i) & 1;

    // Enviar por D7
    digitalWrite(D7_PIN, bitToSend);

    // Pequeño delay para estabilizar señal
    delayMicroseconds(50);

    // Leer D8
    int bitRead = digitalRead(D8_PIN);

    // Validar el bit recibido
    if (bitRead != bitToSend) {
      return "FAIL";  // Error de transmisión
    }

    // Tiempo entre bits
    delayMicroseconds(50);
  }

  return "OK";
}





void melodyBuzzer() {
#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_CS7 2217
#define NOTE_D7 2349
#define NOTE_DS7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_FS7 2960
#define NOTE_G7 3136
#define NOTE_GS7 3322
#define NOTE_A7 3520
#define NOTE_AS7 3729
#define NOTE_B7 3951
#define NOTE_C8 4186
#define NOTE_CS8 4435
#define NOTE_D8 4699
#define NOTE_DS8 4978

#define REST 0

  int melody[] = {
    NOTE_E7, NOTE_E7, REST, NOTE_E7,
    REST, NOTE_C7, NOTE_E7, REST,
    NOTE_G7, REST, REST, REST,
    NOTE_G6, REST, REST, REST,

    NOTE_C7, REST, REST, NOTE_G6,
    REST, REST, NOTE_E6, REST,
    REST, NOTE_A6, REST, NOTE_B6,
    REST, NOTE_AS6, NOTE_A6, REST,

    NOTE_G6, NOTE_E7, NOTE_G7,
    NOTE_A7, REST, NOTE_F7, NOTE_G7,
    REST, NOTE_E7, REST, NOTE_C7,
    NOTE_D7, NOTE_B6, REST, REST
  };

  int durations[] = {
    12, 12, 12, 12,
    12, 12, 12, 12,
    12, 12, 12, 12,
    12, 12, 12, 12,

    12, 12, 12, 12,
    12, 12, 12, 12,
    12, 12, 12, 12,
    12, 12, 12, 12,

    9, 9, 9, 12,
    12, 12, 12, 12,
    12, 12, 12, 12,
    12, 12, 12, 12
  };

  for (int thisNote = 0; thisNote < sizeof(melody) / sizeof(int); thisNote++) {
    int noteDuration = 1000 / durations[thisNote];
    if (melody[thisNote] != REST) {
      tone(BUZZER_PIN, melody[thisNote], noteDuration);
    }
    delay(noteDuration * 1.30);
    noTone(BUZZER_PIN);
  }

  delay(1000);  // Pausa antes de repetir
}
