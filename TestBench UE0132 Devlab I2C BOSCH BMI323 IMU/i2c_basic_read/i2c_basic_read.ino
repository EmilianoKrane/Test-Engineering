#include <DevLab_BMI323.h>

// --------------------------------------------------
// I2C pin configuration
// --------------------------------------------------
#define SDA_PIN 7
#define SCL_PIN 6

// --------------------------------------------------
// Create BMI323 object
// --------------------------------------------------
DevLab_BMI323 imu(Wire, 0x69);

// Structure to store sensor data
BMI323_SensorData data;

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==================================================");
  Serial.println(" DevLab BMI323 - Basic Read Example");
  Serial.println("==================================================");

  Serial.println("Initializing BMI323...");

  // Initialize BMI323
  if (!imu.begin(SDA_PIN, SCL_PIN, 400000)) {

    Serial.println("ERROR: BMI323 initialization failed.");

    while (1) {
      delay(1000);
    }
  }

  Serial.println("BMI323 initialized successfully.");
  Serial.println();
}

// --------------------------------------------------
// Arduino main loop
// --------------------------------------------------
void loop() {

  // Read sensor data
  if (imu.readData(data)) {

    Serial.println("--------------------------------------------------");

    // Accelerometer data
    Serial.print("Accelerometer [raw]");
    Serial.print("  X: ");
    Serial.print(data.accX);

    Serial.print("   Y: ");
    Serial.print(data.accY);

    Serial.print("   Z: ");
    Serial.println(data.accZ);

    // Gyroscope data
    Serial.print("Gyroscope     [raw]");
    Serial.print("  X: ");
    Serial.print(data.gyrX);

    Serial.print("   Y: ");
    Serial.print(data.gyrY);

    Serial.print("   Z: ");
    Serial.println(data.gyrZ);

    // Temperature data
    Serial.print("Temperature   [C]");
    Serial.print("    ");
    Serial.println(data.temperatureC, 2);

  } else {

    Serial.println("ERROR: Failed to read BMI323 data i2c.");
  }

  delay(1000);
}
