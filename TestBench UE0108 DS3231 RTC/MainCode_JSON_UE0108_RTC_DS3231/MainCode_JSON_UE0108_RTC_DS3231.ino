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
