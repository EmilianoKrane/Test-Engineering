# MainCode TestBench Master I2C

Este directorio contiene el firmware del maestro I2C para el sistema TestBench Multi Relay.

## Firmware

- **MainCode_TestBench_Master_I2C.ino**: Código Arduino para Pulsar C6 configurado como maestro I2C. Este firmware se comunica con el esclavo para controlar los relevadores y maneja la interfaz con una página web a través de UART.

## Funcionalidades

El maestro puede:
- Enviar comandos para activar relevadores individuales (canales 1-16)
- Iniciar un barrido automático de todos los relevadores
- Poner el sistema en modo de suspensión
- Escanear dispositivos I2C conectados
- Responder a pings desde la interfaz web
- Gestionar arranques desde un botón físico

## Comunicación

- **I2C**: Comunicación con el esclavo en dirección 0x40
- **UART**: Interfaz con página web para comandos JSON
- **Serial**: Depuración y logs

## Pines Utilizados

- RUN_BUTTON: GPIO 4
- SDA: GPIO 6
- SCL: GPIO 7
- RX2: GPIO 15
- TX2: GPIO 19

## Dependencias

- ArduinoJson
- Wire (I2C)
- HardwareSerial