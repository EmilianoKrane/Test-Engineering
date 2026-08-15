#ifndef DevLab_Test_PulsarC6_H
#define DevLab_Test_PulsarC6_H

#include <Arduino.h>
#include <Wire.h>
#include <FS.h>

// Descomenta y ajusta estas librerías según las que uses en tu proyecto
// #include <ArduinoJson.h>
// #include <Adafruit_NeoPixel.h>

// ==== VARIABLES EXTERNAS ====
// Le decimos al compilador: "Estas variables existen en el main.ino, confía en mí"
extern int intensity;
extern int SDA_PIN;
// extern JsonDocument sendJSON;     // Ajusta el tipo según la versión de ArduinoJson
// extern Adafruit_NeoPixel pixels;  // Descomenta si usas esta librería

// ==== DECLARACIÓN DE FUNCIONES ====

// Debug y Hardware
void serialDebug(String str);
bool testSequence(uint8_t gpioOut, uint8_t gpioIn);
bool testGpios(uint8_t gpioA, uint8_t gpioB);
bool i2cCheckDevice(uint8_t address);
void demo();

// Sistema de Archivos
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void createDir(fs::FS &fs, const char *path);
void removeDir(fs::FS &fs, const char *path);
void readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void renameFile(fs::FS &fs, const char *path1, const char *path2);
void deleteFile(fs::FS &fs, const char *path);
void testFileIO(fs::FS &fs, const char *path);

#endif