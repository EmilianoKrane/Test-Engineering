# TestBench UE0106 - Firmware de prueba

## Firmwares incluidos

- main_JUNR3_blink_passthrough_ue0106_qwiic_converter.ino
  - Firmware del esclavo JUNR3.
  - Gestiona pulsos y lectura ADC sobre sus pines analógicos.
  - Recibe comandos JSON por Serial.

- main_PULSARC6_blink_passthrough_ue0106_qwiic_converter.ino
  - Firmware del maestro PULSARC6.
  - Coordina comunicación con la JUNR3 por UART2.
  - Ejecuta pruebas de ping, blink_out, sweep y blink_in.

## Organización propuesta

### Operaciones de control
- ping
- ping_slave

### Operaciones de salida / estímulo
- blink_out
- blink_in
- blink

### Operaciones de diagnóstico
- readSweep
- sweep

## Notas
- No se modificó la lógica de ejecución del firmware.
- Se añadieron comentarios y una estructura más clara para facilitar mantenimiento.
- Si quieres, el siguiente paso puede ser separar cada operación en funciones para hacerlo aún más legible.
