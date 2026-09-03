/*
Este firmware se  carga en la pulsar c6 para leer la salida del convertidor logico sn74
por medio de un divisor de tensión, el umbral de valores validos se ajusta segun la salida 
obtenida bajando 3.3 a la mitad como estado alto 
*/


#define INPUT_A0 0
#define INPUT_A1 1
#define INPUT_A2 2
#define INPUT_A3 3

float value_a0 = 0, value_a1 = 0, value_a2 = 0, value_a3 = 0;

void setup() {
  pinMode(INPUT_A0, INPUT_PULLDOWN);
  pinMode(INPUT_A1, INPUT_PULLDOWN);
  pinMode(INPUT_A2, INPUT_PULLDOWN);
  pinMode(INPUT_A3, INPUT_PULLDOWN);
}

void loop() {
}
