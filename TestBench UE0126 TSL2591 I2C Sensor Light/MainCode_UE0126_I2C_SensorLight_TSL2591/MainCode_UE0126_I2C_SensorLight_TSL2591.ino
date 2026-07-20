#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_TSL2591.h>

#define RUN_BUTTON 4  // Botón de Arranque
#define SDA_PIN 21
#define SCL_PIN 22
#define RELAYUSB 20  // GPIO020 RELAY USBC

String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;                ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas


//Creacion de objeto de clase Adafruit_TSL2591 que hace referencia al sensor
tsl2591Gain_t ganancias[] = {
  TSL2591_GAIN_LOW,   /// low gain (1x)
  TSL2591_GAIN_MED,   /// medium gain (25x)
  TSL2591_GAIN_HIGH,  /// medium gain (428x)
  TSL2591_GAIN_MAX
};

tsl2591IntegrationTime_t timings[] = {
  TSL2591_INTEGRATIONTIME_100MS,  // 100 millis
  TSL2591_INTEGRATIONTIME_200MS,  // 200 millis
  TSL2591_INTEGRATIONTIME_300MS,  // 300 millis
  TSL2591_INTEGRATIONTIME_400MS,  // 400 millis
  TSL2591_INTEGRATIONTIME_500MS,  // 500 millis
  TSL2591_INTEGRATIONTIME_600MS
};

const char* etiquetasGanancia[] = {
  "LOW",
  "MED",
  "HIGH",
  "MAX"
};

const char* etiquetasTiming[] = {
  "100",
  "200",
  "300",
  "400",
  "500",
  "600"
};

Adafruit_TSL2591 tsl = Adafruit_TSL2591(1);

bool initDevice() {
  bool found = tsl.begin();
  if (found) {
    serialDebug("Dispositivo encontrado en la dirección 0x29");
    return true;
  } else {
    serialDebug("Dispositivo no encontrado");
    return false;
  }
}

void scanIICDevices() {
  Serial.println("Escaneando bus I2C...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("✅ Dispositivo encontrado en: 0x");
      Serial.println(addr, HEX);
    }
  }
}

void sweepMeditions() {
  for (int i = 0; i < 4; i++) {
    tsl.setGain(ganancias[i]);
    for (int j = 0; j < 6; j++) {
      Serial.print(etiquetasGanancia[i]);
      Serial.print(";");
      tsl.setTiming(timings[j]);
      delay((j + 1) * 100 + 20);
      Serial.print(etiquetasTiming[j]);
      Serial.print(";");
      printResults();
    }
  }
}

void advancedRead(void) {
  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  Serial.print(F("[ "));
  Serial.print(millis());
  Serial.print(F(" ms ] "));
  Serial.print(F("IR: "));
  Serial.print(ir);
  Serial.print(F("  "));
  Serial.print(F("Full: "));
  Serial.print(full);
  Serial.print(F("  "));
  Serial.print(F("Visible: "));
  Serial.print(full - ir);
  Serial.print(F("  "));
  Serial.print(F("Lux: "));
  Serial.println(tsl.calculateLux(full, ir), 6);
}


void printResults() {
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;

  //ms;Ganancia;ATIME;CH0;CH1;Visible;Lux
  Serial.print(millis());
  Serial.print(";");
  //Serial.print(tsl.getGain());
  //Serial.print(";");
  //Serial.print(tsl.getTiming());
  //Serial.print(";");
  Serial.print(ir);
  Serial.print(";");
  Serial.print(full);
  Serial.print(";");
  Serial.print(full - ir);
  Serial.print(";");
  Serial.print(tsl.calculateLux(full, ir));
  Serial.println("");
}

void serialDebug(const String& message) {
  String escaped = message;
  escaped.replace("\"", "\\\"");
  Serial.println("{\"debug\": \"" + escaped + "\"}");
}

void setup() {

  Serial.begin(115200);
  delay(100);
  serialDebug("Serial initialized...");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  pinMode(RUN_BUTTON, INPUT);
  pinMode(RELAYUSB, OUTPUT);
  digitalWrite(RELAYUSB, LOW);
}

void loop() {

  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(200);
    sendJSON.clear();  // Limpia cualquier dato previo

    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Arranque por botonera");
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, Serial);  // Envío de datos por JSON a la PagWeb
      Serial.println();
    }
  }


  if (Serial.available()) {
    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    String Function = receiveJSON["Function"];
    int Gain = receiveJSON["i"];
    int Time = receiveJSON["j"];

    int opc = 0;
    if (Function == "ping") opc = 1;       // {"Function" : "ping"}
    else if (Function == "init") opc = 2;  // {"Function" : "init"}
    else if (Function == "read") opc = 3;  // {"Function" : "read", "i":"1", "j":"0"}
    else if (Function == "ON") opc = 4;    // {"Function" : "ON"}
    else if (Function == "OFF") opc = 5;   // {"Function" : "OFF"}

    switch (opc) {
      case 1:
        {
          sendJSON.clear();
          sendJSON["ping"] = "pong";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      case 2:
        {
          sendJSON.clear();
          bool init = tsl.begin();
          delay(200);
          if (init) sendJSON["Result"] = "OK";
          else sendJSON["Result"] = "FAIL";
          delay(200);
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
        }

      case 3:
        {
          sendJSON.clear();
          bool debug = false;
          int i = Gain;  // Ganancia
          int j = Time;  // Tiempos de Integración

          if (i < 4 && j < 6) {
            tsl.setGain(ganancias[i]);
            tsl.setTiming(timings[j]);

            uint32_t lum = tsl.getFullLuminosity();
            uint16_t ir, full, vis;
            ir = lum >> 16;
            full = lum & 0xFFFF;
            vis = full - vis;

            if (debug) {
              // ==== Encabezado ====
              Serial.println("---- GAIN: " + String(etiquetasGanancia[i]) + " ---- " + "TIME: " + String(etiquetasTiming[j]) + " ---- ");
              // ==== Datos ====
              Serial.print("IR: " + String(ir) + " ");
              Serial.print("FULL: " + String(full) + " ");
              Serial.print("VIS: " + String(vis) + " ");
              Serial.println("LUX: " + String(tsl.calculateLux(full, ir)));
              Serial.println(" ");
              delay(500);
            }

            sendJSON["IR"] = ir;
            sendJSON["VIS"] = vis;
            sendJSON["FULL"] = full;
            serializeJson(sendJSON, Serial);
            Serial.println();

          } else serialDebug("Valores de ganancia y tiempo fuera de rango");

          break;
        }

      case 4:
        {
          digitalWrite(RELAYUSB, HIGH);
          break;
        }

      case 5:
        {
          digitalWrite(RELAYUSB, LOW);
          break;
        }

      default: break;
    }
  }
}
