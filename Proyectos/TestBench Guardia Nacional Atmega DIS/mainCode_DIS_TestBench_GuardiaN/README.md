# TestBench Guardia Nacional Atmega DIS — Pulsar Bridge

Descripción
-----------
Firmware para una placa Pulsar que actúa como puente entre el frontend (página web / PagWeb)
y el target Atmega328 del proyecto DIS. La Pulsar recibe comandos JSON por su puerto serial
nativo, traduce/encamina comandos hacia el Atmega328 por UART2 y devuelve estados y resultados
al frontend en formato JSON.

Principales funcionalidades
- Reenvío y recepción de comandos entre frontend y Atmega328.
- Lectura de salidas digitales del target (LED CH1, LED CARGA).
- Pruebas automatizadas (testAll) que verifican estados de GPIO.
- Control simple de un neopixel (encender/apagar con RGB).

Conexiones de hardware
- UART Pulsar <=> Atmega328:
  - Pulsar RX2 (GPIO 4) <- TX del Atmega328
  - Pulsar TX2 (GPIO 5) -> RX del Atmega328
  - Baud rate: 9600, SERIAL_8N1

- Pines de lectura (entradas de la Pulsar):
  - GPIO 2: lectura LED CH1 (PB1 del Atmega -> D9 en target)
  - GPIO 3: lectura LED CARGA (PB2 del Atmega -> D10 en target)

- Botón de arranque:
  - RUN_BUTTON: pin 22 en la Pulsar (lectura digital)

- Neopixel (cadena de matriz)
  - Marrón: GND
  - Rojo: VCC (alimentación del neopixel)
  - Naranja: DATA (señal de datos desde el controlador)

- Multiprotocol / alimentación del target:
  - El multiprotocol debe alimentar el target a 3.3V (NO 5V) para asegurar niveles
    lógicos compatibles y evitar interferencias observadas en D0/D1.

Protocolo JSON soportado (ejemplos)
- Ping:  {"Function":"ping"}
  - Respuesta típica: {"ping":"pong", "PD2_status":0}

- Manage (menú/status):
  - Solicitar ayuda: {"Function":"manage","Action":"help"}
  - Solicitar estado: {"Function":"manage","Action":"status"}

- TestAll: {"Function":"testAll"}
  - Ejecuta secuencia que pone salidas LOW/HIGH y verifica lecturas en GPIOs.

- Neopixel on/off:
  - Encender: {"Function":"neop_ON","R":100,"G":100,"B":100}
  - Apagar:  {"Function":"neop_OFF"}

Notas y recomendaciones
- Use RX/TX en GPIO 4/5 para el UART con Atmega cuando el target esté alimentado a 3.3V,
  ya que se detectó interferencia en D0/D1 bajo 3.3V.
- No modifique la secuencia de operación del firmware sin verificar compatibilidad
  con el frontend y el firmware de prueba del Atmega.
- El firmware ya incluye mensajes de depuración enviados como JSON, visibles en el
  monitor serial a 115200 bps.

Archivos relevantes
- `mainCode_DIS_TestBench_GuardiaN.ino`: código fuente del firmware (comentado).
