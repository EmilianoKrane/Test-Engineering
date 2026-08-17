/*
Este firmware esta probado y validado
*/

#include <SD.h>
#include <SPI.h>

// 1. Pines de la memoria SD (Por defecto VSPI en ESP32)
const int CS_PIN = 5;
const int SCK_PIN = 18;
const int MISO_PIN = 19;
const int MOSI_PIN = 23;

// Pin de la bocina
const int DAC_PIN = 25;

void setup() {
  Serial.begin(115200);
  delay(1000);  // Pequeña pausa para que abra el monitor serie

  Serial.println("Inicializando tarjeta SD...");

  // Configuramos los pines del bus SPI explícitamente
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

  if (!SD.begin(CS_PIN)) {
    Serial.println("Error: Fallo al inicializar la SD o tarjeta no detectada.");
    return;  // Detiene la ejecución si no hay SD
  }

  Serial.println("SD inicializada correctamente.");

  // Imprimir los archivos de la SD
  Serial.println("Archivos en la SD:");
  File root = SD.open("/");
  listarArchivos(root);
  root.close();
}

void loop() {
  // 3. Reproducir (Equivalente al while True)
  // Nota: En Arduino la ruta raíz es "/" en lugar de "/sd/"
  play_wav("/output_8bit_mono.wav", 0.8);
  // play_wav("/saludos_8bits.wav", 0.6);
}

// Función principal de reproducción
void play_wav(const char* filename, float volume) {
  File file = SD.open(filename);

  if (!file) {
    Serial.print("Error abriendo el archivo: ");
    Serial.println(filename);
    delay(2000);  // Pausa antes de reintentar para no saturar el monitor serie
    return;
  }

  // Saltar el encabezado WAV de 44 bytes
  file.seek(44);

  // Leeremos la SD en bloques de 1024 bytes (igual que tu script de Python)
  const int bufferSize = 1024;
  uint8_t buffer[bufferSize];

  while (file.available()) {
    // Lee hasta 1024 bytes en el buffer
    int bytesRead = file.read(buffer, bufferSize);

    // Procesa y reproduce cada byte leído
    for (int i = 0; i < bytesRead; i++) {
      // Ajusta el volumen y asegura que el valor esté entre 0 y 255
      float val = buffer[i] * volume;
      int adjusted_value = constrain((int)val, 0, 255);

      // Escribe el valor analógico en el pin 25
      dacWrite(DAC_PIN, adjusted_value);

      // Pausa para mantener la tasa de 8000 Hz
      delayMicroseconds(125);
    }
  }

  file.close();
  Serial.println("Tono finalizado con exito");
}

// Función auxiliar para imprimir los nombres de los archivos en el Monitor Serie
void listarArchivos(File dir) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;  // Ya no hay más archivos
    }
    Serial.print(" - ");
    Serial.println(entry.name());
    entry.close();
  }
}