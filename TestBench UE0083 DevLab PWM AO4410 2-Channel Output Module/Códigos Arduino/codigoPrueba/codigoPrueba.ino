// LED Fade + PWM duty print on Serial Monitor (simple version)

int PWM_1 = 2;  // PWM pin 1
int PWM_2 = 3;  // PWM pin 2
int STEP    = 5;      // Brightness increment per step

#define RELAYA 8  // Accionamiento de Rele Fuente
#define RELAYB 9

#define RELAYPWM1 D0  // Accionamiento de Rele de conmutación PWM
#define RELAYPWM2 D1

void setup() {
  Serial.begin(115200);          // Match this baud rate in the Serial Monitor
  delay(200);                    // Small pause to avoid garbage at start

  pinMode(RELAYA, OUTPUT);  // Relé de conmutación de fuente
  pinMode(RELAYB, OUTPUT);
  pinMode(RELAYPWM1, OUTPUT);  // Rele de conmutación de PWM
  pinMode(RELAYPWM2, OUTPUT);
  pinMode(PWM_1, OUTPUT);  // Salida de PWM
  pinMode(PWM_2, OUTPUT);

  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);
  digitalWrite(RELAYPWM1, HIGH);
  digitalWrite(RELAYPWM2, HIGH);

  Serial.println("Starting fade...");
}

void loop() {
  // Fade up
  for (int duty = 0; duty <= 255; duty += STEP) {
    analogWrite(PWM_1, duty);
    analogWrite(PWM_2, duty);
    Serial.print("Duty: ");
    Serial.println(duty);
    delay(100);
  }

  // Fade down
  for (int duty = 255; duty >= 0; duty -= STEP) {
    analogWrite(PWM_1, duty);
    analogWrite(PWM_2, duty);
    Serial.print("Duty: ");
    Serial.println(duty);
    delay(100);
  }
}