#include <ArduinoJson.h>
#include <HardwareSerial.h>

// Creación de objeto para UART2 para PULSAR ESP32 C6
HardwareSerial MySerial2(1);

// Pines para UART2 (puedes reasignarlos)
#define RX2 D4    // GPIO9 como RX
#define TX2 D5    // GPIO8 como TX

// Allocate the JSON document
JsonDocument Info;

String json = "";


void setup() {
  // Iniciar UART0 (USB) para depuración
  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");

  // Iniciar UART2 en los pines seleccionados
  MySerial2.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("UART2 iniciado en RX=9, TX=8");


  while (!Serial)
    continue;

  // Add values in the document
 Info["Arg1"] = 0x00; 
 Info["Arg2"] = 0x01; 


  // Add an array.
  JsonArray data = Info["data"].to<JsonArray>();
  data.add(48.756080);
  data.add(2.302038);
}


void loop() {

  if(Serial.available()){
    char c = Serial.read();

    switch(c){
      case 'a': 
        Serial.print("Función de corto");
        //Info["Function"] = "Corto";
        json = "{\"Function\":\"Corto\"}";
        break; 

      case 'b':
        Serial.print("Función de polaridad");
        //Info["Function"] = "Polaridad";
        json= "{\"Function\":\"Polaridad\"}";
        break; 
    }

    //serializeJson(Info, MySerial2);
    MySerial2.println(json);

  }

}