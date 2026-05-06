# MainCode_JSON_UE0127_SensorLightVEML7700

Firmware para el TestBench UE0127 con el sensor de luz VEML7700.

## Descripción

Este proyecto implementa un firmware para ESP32 que:
- Comunica con el sensor VEML7700 por I2C.
- Recibe comandos JSON por UART2 (`PagWeb`).
- Responde con JSON al frontend o al puerto serie.
- Controla un relé USB de 5V para alimentar el arnés de iluminación.
- Detecta un botón de arranque en el testbench.

## Conexiones de hardware

| Señal | Pin ESP32 | Función |
|---|---|---|
| RUN_BUTTON | GPIO4 | Botón de arranque del testbench |
| SDA | GPIO6 | I2C SDA al sensor VEML7700 |
| SCL | GPIO7 | I2C SCL al sensor VEML7700 |
| UART2 RX | GPIO15 | Entrada UART2 desde el frontend PagWeb |
| UART2 TX | GPIO19 | Salida UART2 hacia el frontend PagWeb |
| RELAYUSB | GPIO20 | Relé USB 5V para arnés de iluminación |

## Comunicación serial

- Velocidad: `115200` baudios
- Puerto principal: `PagWeb` usando UART2

## API JSON soportada

### ping

Valida que la comunicación UART funciona.

Petición:
```json
{"Function":"ping"}
```

Respuesta:
```json
{"ping":"pong"}
```

### scanDis

Escanea el bus I2C y envía mensajes de depuración para cada dispositivo encontrado.

Petición:
```json
{"Function":"scanDis"}
```

### initSensor

Inicializa el sensor VEML7700 en el bus I2C.

Petición:
```json
{"Function":"initSensor"}
```

Respuesta esperada:
```json
{"Result":"OK"}
```

### setSensor

Configura el sensor VEML7700 con ganancia e integración.

Petición:
```json
{"Function":"setSensor", "Gain":3, "IntTime":100}
```

Valores válidos para `Gain`:
- `1` = VEML7700_GAIN_1_8
- `2` = VEML7700_GAIN_1_4
- `3` = VEML7700_GAIN_1
- `4` = VEML7700_GAIN_2

Valores válidos para `IntTime`:
- `25`, `50`, `100`, `200`, `400`, `800`

### readSensor

Lee valores de luz promediados y los devuelve en JSON.

Petición:
```json
{"Function":"readSensor"}
```

Respuesta:
```json
{"white":1234.56, "lux":78.90}
```

### relayON / relayOFF

Controla el relé que alimenta el arnés de iluminación.

Petición:
```json
{"Function":"relayON"}
```

```json
{"Function":"relayOFF"}
```

## Notas

- El firmware usa `ArduinoJson` para parsear y generar JSON.
- El sensor VEML7700 se inicializa con la librería `Adafruit_VEML7700`.
- La salida de depuración se envía tanto al puerto serie USB como a UART2.
- Si se desea usar el Serial USB nativo en lugar de `PagWeb`, es necesario ajustar el código y las conexiones de hardware.
