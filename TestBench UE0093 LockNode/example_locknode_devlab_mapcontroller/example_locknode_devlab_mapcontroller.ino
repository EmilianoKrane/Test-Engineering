#include <Arduino.h>
#include <Wire.h>
#include <DevLab_MapController.h>

DevLab_MapController controller;

DevLab_MapController::TestMode currentMode = DevLab_MapController::TEST_MODE_BOTH;
bool running = false;

void printMenu() {
  Serial.println();
  Serial.println("DevLab_MapController Loop Test");
  Serial.println("s = scan");
  Serial.println("1 = PA4 only");
  Serial.println("2 = toggle only");
  Serial.println("3 = toggle + PA4");
  Serial.println("4 = NeoPixel color cycle");
  Serial.println("5 = PWM cycle");
  Serial.println("6 = ADC PA0");
  Serial.println("r = run/stop");
  Serial.println();
}

void startSelectedMode() {
  controller.startContinuousTest(currentMode);
  running = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(6, 7);
  controller.begin(Wire,
                   DevLab_MapController::DEFAULT_SDA,
                   DevLab_MapController::DEFAULT_SCL,
                   DevLab_MapController::DEFAULT_I2C_FREQ);
  controller.initializeI2C(1);
  controller.setTestInterval(1000);
  controller.scanDevices(1);

  printMenu();
  startSelectedMode();
}

void loop() {
  if (running) {
    controller.runContinuousTest();
    delay(controller.getTestInterval());
  }

  if (!Serial.available()) return;

  char cmd = (char)Serial.read();
  while (Serial.available()) Serial.read();

  if (cmd == 's') {
    controller.scanDevices(1);
  } else if (cmd == '1') {
    currentMode = DevLab_MapController::TEST_MODE_PA4_ONLY;
    startSelectedMode();
  } else if (cmd == '2') {
    currentMode = DevLab_MapController::TEST_MODE_TOGGLE_ONLY;
    startSelectedMode();
  } else if (cmd == '3') {
    currentMode = DevLab_MapController::TEST_MODE_BOTH;
    startSelectedMode();
  } else if (cmd == '4') {
    currentMode = DevLab_MapController::TEST_MODE_NEO_CYCLE;
    startSelectedMode();
  } else if (cmd == '5') {
    currentMode = DevLab_MapController::TEST_MODE_PWM_CYCLE;
    startSelectedMode();
  } else if (cmd == '6') {
    currentMode = DevLab_MapController::TEST_MODE_ADC_PA0;
    startSelectedMode();
  } else if (cmd == 'r') {
    if (running) {
      controller.stopContinuousTest();
      running = false;
      controller.showStatistics(1);
    } else {
      startSelectedMode();
    }
  } else {
    printMenu();
  }
}
