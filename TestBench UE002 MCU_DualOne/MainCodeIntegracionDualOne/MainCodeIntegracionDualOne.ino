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
#include <Adafruit_DRV2605.h>


// ==== Declaración de pines ====
// Pines digitales
#define SCL_PIN 21      // SCL pin D2
#define SDA_PIN 20      // SDA pin D3
#define DHT_PIN 9      // Pin DHT D4
#define BUZZER_PIN 11  // GPIO11 D5 para Buzzer en Shield
#define IR_PIN 8       // GPIO8 D6 LED Infrarrojo en Shield
#define D7_PIN 10      // GPIO10 D7 Pin Salida Dig
#define D8_PIN 2       // GPIO2  D8 Pin Entrada Dig
#define RGB_RD09 3     // GPIO3  Rojo RGB Shield D9
#define RGB_GD10 17    // GPIO17 Verde RGB Shield D10
#define RGB_BD11 19    // GPIO19 Azul RGB Shield D11
#define LED_D12 16     // GPIO16 D12 LED Rojo en Shield D12
#define LED_D13 18     // GPIO16 D13 LED Azul en Shield D13

//#define HAP_SDA 20  // GPIO I2C para Haptic Motor
//#define HAP_SCL 21  // GPIO I2C para Haptic Motor

#define LED_NEOP 24  // PIN DEDICADO AL NEOPIXEL WS2812
#define LED_BUIL 25  // Led Builtin GPIO25


// Pines Analógicos
#define A0_PIN 26  // A0 para lectura de potenciometro en Shield
#define A1_PIN 27  // A1 para lectura de LDR en Shield
#define A2_PIN 28  // A2 para lectura de LM35 en Shield
#define A3_PIN 29  // A3 para lectura de sensor de luz TEMT6000

// ==== Creación de objetos ====
Adafruit_NeoPixel np = Adafruit_NeoPixel(1, LED_NEOP, NEO_GRB + NEO_KHZ800);  // Objeto NeoPixel
#define DHTTYPE DHT11                                                         // Tipo de sensor
DHT dht(DHT_PIN, DHTTYPE);                                                    // Objeto Sensor DHT Hum Y Temp
#define OLED_RESET -1                                                         // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128                                                      // OLED display width, in pixels
#define SCREEN_HEIGHT 64                                                      // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);     // Objeto de la OLED
Adafruit_DRV2605 drv;

// ==== Declaración de variables
String stateI2C;
String JSON_entrada;
StaticJsonDocument<400> sendJSON;
StaticJsonDocument<400> receiveJSON;

void setup() {

  Serial.begin(115200);
  Serial.println("UART0 listo para comunicación...");
  Serial1.begin(115200);
  delay(500);

  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();  // Inicialización de I2C de OLED

  /*
  Wire1.setSDA(HAP_SDA);
  Wire1.setSCL(HAP_SCL);
  Wire1.begin();  // Inicialización de I2C de HapticMotor
  */


  analogReadResolution(12);  // Inicialización Resolución del ADC
  np.begin();                // Inicialización de objeto NeoPixel
  dht.begin();               // Inicialización del DHT

  // ==== Chequeo e inicialización de OLED por I2C
  if (!i2cCheckDevice(0x3C)) {
    Serial.println("SSD1306 no encontrada en I2C");
    stateI2C = "FAIL";
  } else {
    Serial.println("SSD1306 detectada");
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    stateI2C = "OK";
  }

  display.clearDisplay();
  // Set text size and color
  display.setTextSize(1.5);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 0);
  display.println(F("Test de Prueba"));
  display.setCursor(30, 10);
  display.println(F("MCU DualOne"));
  display.display();  // Show initial text

  /*
  // ==== Inicialización de Haptic Motor
  if (!drv.begin(&Wire1)) {
    Serial.println("No se encontró DRV2605 en I2C1!");
  }
  //drv.begin();           // Inicialización de HapticMotor
  drv.selectLibrary(1);  // Librería de efectos
  drv.useERM();          // ERM por defecto
  drv.setMode(DRV2605_MODE_INTTRIG);
*/


  // ==== Declaración de GPIOs
  // ==== Salidas
  pinMode(LED_BUIL, OUTPUT);    // Led Builtin GPIO25
  pinMode(BUZZER_PIN, OUTPUT);  // BUZZER D5
  pinMode(D7_PIN, OUTPUT);      // Salida Digital D7
  pinMode(RGB_RD09, OUTPUT);    // D9 RGB en Shield
  pinMode(RGB_GD10, OUTPUT);    // D10 RGB en Shield
  pinMode(RGB_BD11, OUTPUT);    // D11 RGB en Shield
  pinMode(LED_D12, OUTPUT);     // D12 LED Rojo en Shield
  pinMode(LED_D13, OUTPUT);     // D13 LED Azul en Shield

  // ==== Entradas
  pinMode(IR_PIN, INPUT);  // IR LED D6 en Shield
  pinMode(D8_PIN, INPUT);  // Entrada Digital D8
  pinMode(A0_PIN, INPUT);  // A0 para potenciometro en Shield
  pinMode(A1_PIN, INPUT);  // A1 para LDR en Shield
  pinMode(A2_PIN, INPUT);  // A2 para LM35 en Shield
  pinMode(A3_PIN, INPUT);  // A3 para sensor TEMT6000
}

void loop() {

  if (Serial1.available()) {  // Rutina de chequeo de GPIOs

    JSON_entrada = Serial1.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "meas") opc = 1;
      else if (Function == "buzzer") opc = 2;


      switch (opc) {

        // Condición de testeo general de GPIOs
        case 1:
          {
            sendJSON.clear();
            // ==== Constructor OLED ====
            display.clearDisplay();
            display.setCursor(0, 0);
            display.setTextSize(1);
            display.print(F("Rutina: "));
            display.println("Check GPIOs");
            display.display();
            // ==== Constructor OLED ====

            // Validación D0 y D1
            // Validación D2 y D3 OLED
            sendJSON["D2"] = stateI2C;
            sendJSON["D3"] = stateI2C;


            // Validación D4 Sensor Hum Y Temp en Shield
            display.setCursor(0, 10);
            String statusD4 = DHTmeas();
            writeLCD("D4: " + statusD4, 0);
            sendJSON["D4"] = statusD4;

            // Validación D5 Buzzer en Shield
            melodyBuzzer();

            // Validación D6 IR Receptor en Shield
            display.setCursor(0, 20);
            String statusD6 = IRLEDreceptor();
            writeLCD("D6: " + statusD6, 0);
            sendJSON["D6"] = statusD6;

            // Validación D7 y D8 en Shield
            String statusD78 = sequenceDIG();
            display.setCursor(0, 30);
            writeLCD("D7: " + statusD78, 0);
            sendJSON["D7"] = statusD78;

            display.setCursor(0, 40);
            writeLCD("D8: " + statusD78, 0);
            sendJSON["D8"] = statusD78;

            // Validación D9 - D11 RGB LED Shield
            // Validación D12 - D13 LEDs en Shield

            //  Validación GPIOs 20 21 I2C
            //HapticMotor();

            // ==== VALIDACIÓN DE PINES ANALÓGICOS =====
            display.setCursor(60, 10);
            display.setTextSize(1);

            // Validación A0 Potenciómetro en Shield
            String statusA0 = analogA0();
            writeLCD("A0: " + statusA0, 0);
            sendJSON["A0"] = statusA0;

            display.setCursor(60, 20);
            display.setTextSize(1);

            // Validación A1 LDR en Shield
            String statusA1 = analogA1();
            writeLCD("A1: " + statusA1, 0);
            sendJSON["A1"] = statusA1;

            display.setCursor(60, 30);
            display.setTextSize(1);

            // Validación A2 LM35 en Shield
            String statusA2 = analogA2();
            writeLCD("A2: " + statusA2, 0);
            sendJSON["A2"] = statusA2;

            display.setCursor(60, 40);
            display.setTextSize(1);

            // Validación A3 Sensor de Luz TEMT6000
            String statusA3 = analogA3();
            writeLCD("A3: " + statusA3, 0);
            sendJSON["A3"] = statusA3;


            serializeJson(sendJSON, Serial1);  // Envío de datos por JSON a la ESP32
            Serial1.println();
            break;
          }

        case 2:
          {
            // Validación D5 Buzzer en Shield
            melodyBuzzer();
            break;
          }
      }
    }
  } else {
    // Rutina de demo
    sequenceNeop();
  }
}




// ==== DECLARACIÓN DE FUNCIONES AUXILIARES ====

// Secuencia Demo de Neopixel y LED Bultin
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
  digitalWrite(LED_D12, HIGH);
  digitalWrite(LED_D13, LOW);

  delay(500);

  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(0, 255, 0));
  np.show();
  delay(500);
  digitalWrite(LED_BUIL, LOW);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, HIGH);
  digitalWrite(RGB_BD11, LOW);
  digitalWrite(LED_D12, LOW);
  digitalWrite(LED_D13, HIGH);

  delay(500);

  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(0, 0, 255));
  np.show();
  delay(500);
  digitalWrite(LED_BUIL, LOW);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, LOW);
  digitalWrite(RGB_BD11, HIGH);
  digitalWrite(LED_D12, LOW);
  digitalWrite(LED_D13, HIGH);

  delay(500);

  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(0, 0, 0));
  np.show();
  delay(500);
  digitalWrite(LED_BUIL, LOW);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, LOW);
  digitalWrite(RGB_BD11, LOW);
  digitalWrite(LED_D12, HIGH);
  digitalWrite(LED_D13, LOW);

  delay(500);
}


// Función de escritura de status en OLED D2 y D3
bool i2cCheckDevice(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);
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

// Función de reconocimiento y lectura de sensor DHT Temp y Hum D4
String DHTmeas() {
  Serial.println("==== Lectura Sensor en D4 GPIO09 ====");
  float h = dht.readHumidity();
  float t = dht.readTemperature();  // °C

  if (isnan(h) || isnan(t)) {
    Serial.println("Error leyendo el DHT11");
    return "FAIL";
  }

  Serial.print("Temperatura: ");
  Serial.print(t);
  Serial.print(" °C\t");

  Serial.print("Humedad: ");
  Serial.print(h);
  Serial.println(" %");

  return "OK";
}

// Función de melodía en Buzzer D5
void melodyBuzzer() {

  Serial.println("==== Ejecución de rutina en Buzzer D5 ====");

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

// Función de lectura digital en GPIO8 D6
String IRLEDreceptor() {
  Serial.println("==== Lectura de sensor IR en D6 GPIO08 ====");
  int n = 0;
  int lect = 0;
  for (int i = 0; i < 10; i++) {
    n = digitalRead(IR_PIN);
    lect += n;
  }

  if (lect == 10) {
    Serial.println("Lectura HIGH estable en D6: " + String(lect) + "/10");
    return "OK";
  } else {
    Serial.println("Lectura inestable en D6: " + String(lect) + "/10");
    return "FAIL";
  }
}

// Función de escritura y lectura de secuencia en pines digitales D7 y D8
String sequenceDIG() {
  byte testSequence = 0b10101100;
  for (int i = 7; i >= 0; i--) {
    // Obtener el bit i
    int bitToSend = (testSequence >> i) & 1;

    // Enviar secuencia por D7
    digitalWrite(D7_PIN, bitToSend);
    delay(50);

    // Leer secuencia por D8
    int bitRead = digitalRead(D8_PIN);

    // Validar el bit recibido
    if (bitRead != bitToSend) {
      return "FAIL";  // Error de transmisión
    }
    delay(50);
  }
  return "OK";
}

// Función de Haptic Motor GPIO20 y 21
void HapticMotor() {
  for (int i = 0; i < 10; i++) {
    drv.setWaveform(0, 118);  // vibración fuerte
    drv.setWaveform(1, 0);    // fin
    drv.go();
    delay(300);

    drv.setWaveform(0, 47);  // pulso corto
    drv.setWaveform(1, 0);
    drv.go();
  }
}


String analogA0() {
  float n = 0;
  float lect = 0;
  for (int i = 0; i < 10; i++) {
    n = analogRead(A0_PIN);
    lect += n;
  }
  float avg = lect / 10;
  Serial.println("El promedio de lecturas en A0: " + String(avg));
  if (avg > 1000) {
    return "OK";
  } else {
    return "FAIL";
  }
}

String analogA1() {
  float n = 0;
  float lect = 0;
  for (int i = 0; i < 10; i++) {
    n = analogRead(A1_PIN);
    lect += n;
  }
  float avg = lect / 10;
  Serial.println("El promedio de lecturas en A1: " + String(avg));
  if (avg > 1000) {
    return "OK";
  } else {
    return "FAIL";
  }
}

String analogA2() {
  float n = 0;
  float lect = 0;
  for (int i = 0; i < 10; i++) {
    n = analogRead(A2_PIN);
    lect += n;
  }
  float avg = lect / 10;
  Serial.println("El promedio de lecturas en A2: " + String(avg));
  if (avg > 1000) {
    return "OK";
  } else {
    return "FAIL";
  }
}

String analogA3() {
  float n = 0;
  float lect = 0;
  for (int i = 0; i < 10; i++) {
    n = analogRead(A3_PIN);
    lect += n;
  }
  float avg = lect / 10;
  Serial.println("El promedio de lecturas en A3: " + String(avg));
  if (avg > 1000) {
    return "OK";
  } else {
    return "FAIL";
  }
}
