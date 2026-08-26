
#define LED_BUILTIN = 22;  // D13/BUILTIN1 in the V1.3 schematic

int delay_ms = 200;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Blink test started");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("LED ON");
  delay(delay_ms);

  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("LED OFF");
  delay(delay_ms);
}