# Main Slave

Este directorio contiene el firmware para el dispositivo esclavo en el sistema TestBench Multi Relay.

## Firmware

- **main.py**: Firmware escrito en MicroPython para ESP32C6 configurado como esclavo I2C. Este código maneja la recepción de comandos del maestro y controla el multiplexador de relevadores.

## Funcionalidad

El esclavo responde a comandos I2C enviados por el maestro para:
- Activar relevadores individuales (comandos 0x00 - 0x0F para canales 1-16)
- Realizar un barrido automático (comando 0xFF)
- Entrar en modo de suspensión (comando 0xFE)

## Notas

- No modificar el archivo main.py directamente.
- Asegurarse de que la dirección I2C del esclavo coincida con la configurada en el maestro (0x40).