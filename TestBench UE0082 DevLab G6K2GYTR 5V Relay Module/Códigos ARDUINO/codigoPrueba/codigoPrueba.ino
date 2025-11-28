// --------------------------------------------------
// Relay Blink with On/Off Messages
// --------------------------------------------------

int IN_PIN  = 23;  // Digital pin where the IN pin is connected
int T = 1000;     // Interval in milliseconds

void setup() {
  // Initialize the pin as an output|
  pinMode(IN_PIN, OUTPUT);

  // Initialize the Serial port at 9600 baud
  Serial.begin(9600);
  while (!Serial) ;  // Wait for the Serial Monitor to open (only on boards that require it)
  Serial.print("No está habilitado el Serie");
}

void loop() {
  // --- HIGH ---
  digitalWrite(IN_PIN, HIGH);
  Serial.print("NO: On ");
  Serial.println("NC: Off");
  delay(T);

  // --- LOW ---
  digitalWrite(IN_PIN, LOW);
  Serial.print("NO: Off ");
  Serial.println("NC: On");
  delay(T);
}
