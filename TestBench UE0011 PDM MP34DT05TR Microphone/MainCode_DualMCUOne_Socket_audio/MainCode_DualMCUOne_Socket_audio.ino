/*
 * Arduino example: PDM microphone capture on RP2040/RP2350
 *
 * This sketch mirrors the pico-sdk hello_pdm_microphone example:
 * it captures audio and prints signed PCM samples over Serial.
 */

#include <Arduino.h>
#include <PDM.h>

// Match the pico-sdk example defaults
static const int kSampleRate = 8000;
static const int kChannels = 1;
static const size_t kSampleBufferCount = 256;
static const int kPdmDinPin = 4;
static const int kPdmClkPin = 5;

static int16_t sampleBuffer[kSampleBufferCount];
static volatile size_t samplesRead = 0;

void onPdmData() {
  const int bytesAvailable = PDM.available();
  if (bytesAvailable <= 0) {
    return;
  }

  const int bytesToRead = min(bytesAvailable, (int)sizeof(sampleBuffer));
  const int bytesRead = PDM.read((void*)sampleBuffer, bytesToRead);
  if (bytesRead > 0) {
    samplesRead = (size_t)bytesRead / sizeof(sampleBuffer[0]);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  // For arduino-pico RP2040/RP2350 core, pins must be set before begin().
  PDM.setDIN(kPdmDinPin);
  PDM.setCLK(kPdmClkPin);

  PDM.onReceive(onPdmData);
  PDM.setBufferSize(sizeof(sampleBuffer));

  if (!PDM.begin(kChannels, kSampleRate)) {
    Serial.println("PDM microphone initialization failed!");
    Serial.println("Check wiring: DIN=GPIO11 CLK=GPIO10");
    Serial.println("If using another pinout, edit kPdmDinPin/kPdmClkPin");
    Serial.println("If using non-rp2040 core, verify PDM API compatibility");
    while (true) {
      delay(100);
    }
  }

  Serial.println("hello PDM microphone (Arduino)");
}

void loop() {
  if (samplesRead == 0) {
    delay(1);
    return;
  }

  noInterrupts();
  size_t localCount = samplesRead;
  samplesRead = 0;
  interrupts();

  if (localCount > kSampleBufferCount) {
    localCount = kSampleBufferCount;
  }

  // Enviar TODO el buffer en binario de una sola vez
  // en lugar de usar un for y Serial.println()
  Serial.write((uint8_t*)sampleBuffer, localCount * sizeof(int16_t));
}