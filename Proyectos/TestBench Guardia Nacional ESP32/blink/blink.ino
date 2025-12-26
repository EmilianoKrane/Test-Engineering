// MainCode TestPruebas ATMega328p Guardia Nacional DIS

#include <Wire.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <HardwareSerial.h>

// === Declaración de pines ===
// Pines de Testeo en AtMega328 GN
#define NEOPIXEL 8   // Salida al Neopixel
#define LED_CH1 9    // PIN LEDCH1 PB1  Entrada
#define LED_CHG 10   // PIN LEDcarga PB2 Salida
#define valueADC A0  // ADC en PC0
#define SWITCH 2     // Interrupción del Switch de Buzzer
#define FAN 3        // PIN PD3 activación de ventilador
#define XD 4         // PIN PD4 activa un MOSFET rancio

// Pines de simulación JUNR3
#define LED_BUILTIN 13  // Blink en JUNR3

// === Creación de Objetos ===
Adafruit_NeoPixel pixels(25, NEOPIXEL, NEO_GRB + NEO_KHZ800);  // Objeto NeoPixel

// === Declaración de JSON ===
// JSON para recibir datos
String JSON_receive;                // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> datosJSON;  // Usa StaticJsonDocument para fijar tamaño

// JSON para enviar datos
String JSON_send;
StaticJsonDocument<200> enviarJSON;

void setup() {

  Serial.begin(115200);
  Serial.println("Serial inicializado en D0 y D1...");

  pixels.begin();
  pixels.setBrightness(2);
  pixels.clear();
  pixels.show();

  // Declaración de entradas/salidas
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(NEOPIXEL, OUTPUT);
  pinMode(LED_CHG, OUTPUT);

  pinMode(LED_CH1, INPUT_PULLUP);
  pinMode(SWITCH, INPUT);

  digitalWrite(LED_CHG, LOW);
}

void loop() {

  if (Serial.available()) {

    JSON_receive = Serial.readStringUntil('\n');

    // Deserializa el JSON y guarda la información en datosJSON
    DeserializationError error = deserializeJson(datosJSON, JSON_receive);

    if (!error) {
      String Function = datosJSON["Function"];

      int opc = 0;

      if (Function == "testAll") opc = 1;
      else if (Function == "neop") opc = 2;
      else if (Function == "adc") opc = 3;
      else if (Function == "dig") opc = 4;

      // {"Function":"testAll"}

      switch (opc) {
        case 1:
          {
            enviarJSON.clear();  // Limpia cualquier dato previo

            // Validación de pines PB1-PB2 LEDCH1-CHG
            byte secuencia[] = { 1, 0, 1, 1, 0, 0, 1, 0 };
            const int lenSeq = sizeof(secuencia);
            bool status_gpio = true;

            for (int i = 0; i < lenSeq; i++) {
              digitalWrite(LED_CHG, secuencia[i]);
              delay(50);
              int lectura = digitalRead(LED_CH1);
              Serial.println("Valor [" + String(i) + "]: " + String(lectura));

              if (digitalRead(LED_CH1) != secuencia[i]) {
                status_gpio = false;
                Serial.print("Error en índice ");
                Serial.println(i);
              }
              delay(50);
            }

            if (status_gpio) {
              Serial.println("SECUENCIA CORRECTA");
              enviarJSON["gpio"] = "OK";
            } else {
              Serial.println("SECUENCIA INCORRECTA");
              enviarJSON["gpio"] = "Fail";
            }
            Serial.println("");

            // Validación de Analógicos A0
            const int tolerancia = 10;
            int ref = -1;
            bool estable = true;

            for (int i = 0; i < 10; i++) {
              int lectura = analogRead(valueADC);
              Serial.println("Lectura A0 " + String(i) + ": " + String(lectura));

              if (i == 0) {
                ref = lectura;  // primera lectura como referencia
              } else {
                if (abs(lectura - ref) > tolerancia) {
                  estable = false;
                }
              }
              delay(5);  // evita leer “ruido” instantáneo
            }

            if (estable && ref < 100) {
              enviarJSON["analog"] = "OK";
            } else {
              enviarJSON["analog"] = "Fail";
            }
            Serial.println("");

            // Validación de interrupción por switch en PD2 D2
            // Incluir buzzer por PD4 para validar
            bool statusINT = false;
            /*
            for (int i = 0; i < 50; i++) {

              if (digitalRead(SWITCH) == LOW) {
                delay(50);
                if (digitalRead(SWITCH) == HIGH) {
                  delay(50);
                  if (digitalRead(SWITCH) == LOW) {
                    enviarJSON["sw"] = "OK";
                    statusINT = true;
                    break;
                  }
                }
              }
              delay(100);
            }
            */

            if (digitalRead(SWITCH) == LOW) {
              enviarJSON["sw"] = "OK";
              statusINT = true;
            }

            if (!statusINT) {
              enviarJSON["sw"] = "Fail";
            }

            // Validación PD4 a Ventilador
            melodyBuzzer();


            serializeJson(enviarJSON, Serial);  // Envío de datos por JSON a la PagWeb
            Serial.println();                   // Salto de línea para delimitar
            break;
          }

        case 2:
          enviarJSON.clear();  // Limpia cualquier dato previo
          enviarJSON["Case2"] = "OK";
          serializeJson(enviarJSON, Serial);  // Envío de datos por JSON a la PagWeb
          Serial.println();                   // Salto de línea para delimitar
          break;

        case 3:
          enviarJSON.clear();  // Limpia cualquier dato previo
          enviarJSON["Case3"] = "OK";
          serializeJson(enviarJSON, Serial);  // Envío de datos por JSON a la PagWeb
          Serial.println();                   // Salto de línea para delimitar
          break;

        case 4:
          enviarJSON.clear();  // Limpia cualquier dato previo
          enviarJSON["Case4"] = "OK";
          serializeJson(enviarJSON, Serial);  // Envío de datos por JSON a la PagWeb
          Serial.println();                   // Salto de línea para delimitar
          break;
      }
    }
  } else {
    sequenceNeop();
  }
}



// ==== Funciones de ejecución ====

void sequenceNeop() {
  colorWipe(pixels.Color(255, 0, 0));
  delay(200);

  colorWipe(pixels.Color(0, 255, 0));
  delay(200);

  colorWipe(pixels.Color(245, 73, 39));
  delay(200);

  colorWipe(pixels.Color(0, 0, 255));
  delay(200);
}

void colorWipe(uint32_t color) {
  for (int i = 0; i < 25; i++) {
    pixels.setPixelColor(i, color);
    pixels.show();
    delay(50);
  }
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
      tone(FAN, melody[thisNote], noteDuration);
    }
    delay(noteDuration * 1.30);
    noTone(FAN);
  }

  delay(1000);  // Pausa antes de repetir
}
