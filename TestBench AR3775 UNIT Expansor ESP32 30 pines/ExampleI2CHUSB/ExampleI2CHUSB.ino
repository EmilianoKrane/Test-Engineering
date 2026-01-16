#include <Wire.h>
#include "Adafruit_HUSB238.h"

#define I2C_SDA 22
#define I2C_SCL 23
 
Adafruit_HUSB238 husb238;
String cmd = "";

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
    Wire.begin(I2C_SDA, I2C_SCL);

  Serial.println("Adafruit HUSB238 Test Sketch");

  // Initialize the HUSB238
  if (husb238.begin(HUSB238_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("HUSB238 initialized successfully.");
  } else {
    Serial.println("Couldn't find HUSB238, check your wiring?");
    while (1);
  }
}

void loop() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n') {
      handleSCPI(cmd);
      cmd = "";
    } else {
      cmd += ch;
    }
  }
}

void handleSCPI(String c) {
  c.trim();
  c.toUpperCase();

  if (c == "*IDN?") {
    Serial.println("UNIT-DEVLAB,HUSB238,USBPD,1.0");
  }

  else if (c == "STAT?") {
    Serial.println(husb238.isAttached() ? "ATTACHED" : "UNATTACHED");
  }

  else if (c == "PD:LIST?") {
    HUSB238_PDSelection voltages[] = {PD_SRC_5V, PD_SRC_9V, PD_SRC_12V, PD_SRC_15V, PD_SRC_18V, PD_SRC_20V};
    int voltageValues[] = {5, 9, 12, 15, 18, 20};
    
    for (int i = 0; i < 6; i++) {
      if (husb238.isVoltageDetected(voltages[i])) {
        Serial.print(voltageValues[i]);
        Serial.print("V ");
      }
    }
    Serial.println();
  }

  else if (c == "PD:GET?") {
    Serial.print("PD=");
    Serial.println(husb238.getPDSrcVoltage());
  }

  else if (c.startsWith("PD:SET")) {
    int v = c.substring(6).toInt();
    HUSB238_PDSelection sel;

    switch (v) {
      case 5:  sel = PD_SRC_5V; break;
      case 9:  sel = PD_SRC_9V; break;
      case 12: sel = PD_SRC_12V; break;
      case 15: sel = PD_SRC_15V; break;
      case 18: sel = PD_SRC_18V; break;
      case 20: sel = PD_SRC_20V; break;
      default:
        Serial.println("ERR:INVALID_VOLTAGE");
        return;
    }

    if (husb238.isVoltageDetected(sel)) {
      husb238.selectPD(sel);
      husb238.requestPD();
      Serial.print("OK:SET ");
      Serial.print(v);
      Serial.println("V");
    } else {
      Serial.println("ERR:UNAVAILABLE");
    }
  }

  else if (c == "PD:SWEEP") {
    HUSB238_PDSelection levels[] = {
      PD_SRC_5V, PD_SRC_9V, PD_SRC_12V,
      PD_SRC_15V, PD_SRC_18V, PD_SRC_20V
    };

    for (int i = 0; i < 6; i++) {
      if (husb238.isVoltageDetected(levels[i])) {
        husb238.selectPD(levels[i]);
        husb238.requestPD();
        Serial.print("SWEEP ");
        Serial.print((i+1)*5);
        Serial.println("V");
        delay(1500);
      }
    }
    Serial.println("SWEEP DONE");
  }

  else if (c == "CURR:GET?") {
    HUSB238_CurrentSetting curr = husb238.getPDSrcCurrent();
    Serial.print("CURR=");
    printCurrentValue(curr);
    Serial.println();
  }

  else if (c.startsWith("CURR:MAX?")) {
    int v = c.substring(9).toInt();
    HUSB238_PDSelection sel;

    switch (v) {
      case 5:  sel = PD_SRC_5V; break;
      case 9:  sel = PD_SRC_9V; break;
      case 12: sel = PD_SRC_12V; break;
      case 15: sel = PD_SRC_15V; break;
      case 18: sel = PD_SRC_18V; break;
      case 20: sel = PD_SRC_20V; break;
      default:
        Serial.println("ERR:INVALID_VOLTAGE");
        return;
    }

    if (husb238.isVoltageDetected(sel)) {
      HUSB238_CurrentSetting curr = husb238.currentDetected(sel);
      Serial.print("MAX_CURR@");
      Serial.print(v);
      Serial.print("V=");
      printCurrentValue(curr);
      Serial.println();
    } else {
      Serial.println("ERR:UNAVAILABLE");
    }
  }

  else {
    Serial.println("ERR:UNKNOWN_CMD");
  }
}

void printCurrentValue(HUSB238_CurrentSetting curr) {
  switch (curr) {
    case CURRENT_0_5_A:  Serial.print("0.5A"); break;
    case CURRENT_0_7_A:  Serial.print("0.7A"); break;
    case CURRENT_1_0_A:  Serial.print("1.0A"); break;
    case CURRENT_1_25_A: Serial.print("1.25A"); break;
    case CURRENT_1_5_A:  Serial.print("1.5A"); break;
    case CURRENT_1_75_A: Serial.print("1.75A"); break;
    case CURRENT_2_0_A:  Serial.print("2.0A"); break;
    case CURRENT_2_25_A: Serial.print("2.25A"); break;
    case CURRENT_2_50_A: Serial.print("2.50A"); break;
    case CURRENT_2_75_A: Serial.print("2.75A"); break;
    case CURRENT_3_0_A:  Serial.print("3.0A"); break;
    case CURRENT_3_25_A: Serial.print("3.25A"); break;
    case CURRENT_3_5_A:  Serial.print("3.5A"); break;
    case CURRENT_4_0_A:  Serial.print("4.0A"); break;
    case CURRENT_4_5_A:  Serial.print("4.5A"); break;
    case CURRENT_5_0_A:  Serial.print("5.0A"); break;
    default: Serial.print("UNKNOWN"); break;
  }
}


void printCurrentSetting(HUSB238_CurrentSetting srcCurrent) {
  switch (srcCurrent) {
    case CURRENT_0_5_A:
      Serial.print("0.5A ");
      break;
    case CURRENT_0_7_A:
      Serial.print("0.7A ");
      break;
    case CURRENT_1_0_A:
      Serial.print("1.0A ");
      break;
    case CURRENT_1_25_A:
      Serial.print("1.25A ");
      break;
    case CURRENT_1_5_A:
      Serial.print("1.5A ");
      break;
    case CURRENT_1_75_A:
      Serial.print("1.75A ");
      break;
    case CURRENT_2_0_A:
      Serial.print("2.0A ");
      break;
    case CURRENT_2_25_A:
      Serial.print("2.25A ");
      break;
    case CURRENT_2_50_A:
      Serial.print("2.50A ");
      break;
    case CURRENT_2_75_A:
      Serial.print("2.75A ");
      break;
    case CURRENT_3_0_A:
      Serial.print("3.0A ");
      break;
    case CURRENT_3_25_A:
      Serial.print("3.25A ");
      break;
    case CURRENT_3_5_A:
      Serial.print("3.5A ");
      break;
    case CURRENT_4_0_A:
      Serial.print("4.0A ");
      break;
    case CURRENT_4_5_A:
      Serial.print("4.5A ");
      break;
    case CURRENT_5_0_A:
      Serial.print("5.0A ");
      break;
    default:
      break;
  }
}