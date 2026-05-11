# MainCode_JSON_AR0150_4Mods_LM2596

Firmware para el TestBench del regulador LM2596.
Este firmware actúa como puente entre la interfaz de pruebas y el hardware, permitiendo ejecutar pruebas de hasta 4 módulos LM2596 mediante comandos JSON y relés.

## Descripción

- Comunica con la interfaz web a través de UART2.
- Controla relés para cortocircuito y alimentación.
- Usa sensores INA219 para leer corriente en la salida del testbench.
- Envía y recibe comandos JSON para operaciones de prueba.

## Hardware

Pines principales:
- `RUN_BUTTON` = GPIO4: botón físico de arranque.
- `SDA_PIN` = GPIO6: línea SDA del bus I2C.
- `SCL_PIN` = GPIO7: línea SCL del bus I2C.
- `RX2` = GPIO15: RX de UART2 hacia la interfaz web.
- `TX2` = GPIO19: TX de UART2 hacia la interfaz web.
- `RELAY1` = GPIO14: relé de cortocircuito 1.
- `RELAY2` = GPIO0: relé de cortocircuito 2.
- `RELAYA` = GPIO8: relé de fuente de alimentación +.
- `RELAYB` = GPIO9: relé de fuente de alimentación -.

## Sensores

- `ina219_in` en dirección I2C `0x40` (entrada).
- `ina219_out` en dirección I2C `0x41` (salida).

## Configuración UART

- Velocidad: `115200`.
- Formato: `SERIAL_8N1`.
- Comunicación con la interfaz web usando el objeto `PagWeb`.

## Comandos JSON admitidos

- `{"Function": "ping"}`
  - Responde con `{"ping":"pong"}`.
- `{"Function": "scanAddr"}`
  - Escanea direcciones I2C y muestra dispositivos encontrados.
- `{"Function": "channelON", "channel": <1..16>}`
  - Activa un canal de prueba específico.
- `{"Function": "sweep"}`
  - Inicia un barrido automático de prueba.
- `{"Function": "sleep"}`
  - Pone el testbench en modo suspensión.
- `{"Function": "shortCircuit"}`
  - Ejecuta prueba de cortocircuito y envía la lectura de corriente.

## Notas

- El objeto `I2CBus` existe como reserva, pero en esta versión el sensor INA219 usa el bus `Wire`.
