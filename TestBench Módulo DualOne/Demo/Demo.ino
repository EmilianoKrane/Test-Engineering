/*
#define DIR D1
#define PUL D0

#define highStop 4
#define lowStop 5

void setup() {
  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");

  pinMode(DIR, OUTPUT);
  pinMode(PUL, OUTPUT);

  pinMode(highStop, INPUT);
  pinMode(lowStop, INPUT);
}

void loop() {

  for (int i = 1; i < 700; i++) {
    digitalWrite(DIR, LOW);

    digitalWrite(PUL, HIGH);
    delayMicroseconds(500);
    digitalWrite(PUL, LOW);
    delayMicroseconds(500);

    int Lect1 = digitalRead(highStop);
    Serial.println("Stop ALTO: " + String(Lect1));
    int Lect2 = digitalRead(lowStop);
    Serial.println("Stop BAJO: " + String(Lect2));
  }

  for (int i = 1; i < 700; i++) {
    digitalWrite(DIR, HIGH);

    digitalWrite(PUL, HIGH);
    delayMicroseconds(500);  // 0.5 ms
    digitalWrite(PUL, LOW);
    delayMicroseconds(500);  // 0.5 ms
  }
}

*/


#define DIR D1
#define PUL D0

#define highStop 4
#define lowStop 5

enum Estado {
  SUBIENDO,
  BAJANDO
};

Estado estado = SUBIENDO;   // Arranca subiendo, si quieres

void setup() {
  Serial.begin(115200);

  pinMode(DIR, OUTPUT);
  pinMode(PUL, OUTPUT);

  pinMode(highStop, INPUT);
  pinMode(lowStop, INPUT);
}

void stepPulse() {
  digitalWrite(PUL, HIGH);
  delayMicroseconds(500);
  digitalWrite(PUL, LOW);
  delayMicroseconds(500);
}

void loop() {

  int high = digitalRead(highStop);
  int low  = digitalRead(lowStop);

  Serial.print("HighStop:");
  Serial.print(high);
  Serial.print(" LowStop:");
  Serial.print(low);
  Serial.print(" Estado:");
  Serial.println(estado == SUBIENDO ? "Subiendo" : "Bajando");

  // ======================
  //     ESTADO SUBIENDO
  // ======================
  if (estado == SUBIENDO) {

    digitalWrite(DIR, HIGH);   // subir

    // Si llegó al tope superior → cambiar de estado
    if (high == 0) {
      estado = BAJANDO;
      return;
    }

    // Si no ha llegado → seguir subiendo
    stepPulse();
    return;
  }

  // ======================
  //     ESTADO BAJANDO
  // ======================
  if (estado == BAJANDO) {

    digitalWrite(DIR, LOW);    // bajar

    // Si llegó al tope inferior → cambiar de estado
    if (low == 0) {
      estado = SUBIENDO;
      return;
    }

    // Si no ha llegado → seguir bajando
    stepPulse();
    return;
  }
}
