# DualMCUOne Socket Audio - Firmware PDM MP34DT05TR Microphone

## 📋 Descripción General

Este proyecto es un **sistema de captura y transmisión de audio en tiempo real** basado en la placa **DualMCUOne con RP2040**. El firmware captura audio desde un micrófono PDM MP34DT05TR y envía los datos a través de puerto serial a un servidor WebSocket Python, que luego retransmite los datos a un frontend web para su visualización y procesamiento.

### Arquitectura del Sistema

```
Micrófono PDM MP34DT05TR
        ↓
    RP2040 (DualMCUOne)
    [Firmware Arduino]
        ↓
    Puerto Serial (115200 bps)
        ↓
    Script Python (audio.py)
    [Servidor WebSocket]
        ↓
    Frontend Web
    [Cliente WebSocket]
```

---

## 🔧 Hardware Requerido

- **Placa**: DualMCUOne con RP2040
- **Micrófono**: MP34DT05TR (PDM - Pulse Density Modulation)
- **Conectividad**: Puerto USB para Serial + Ethernet/WiFi (opcional para frontend)
- **Suministro de Energía**: USB o fuente externa de 5V

---

## 📌 Conexiones del Micrófono PDM

Las conexiones configuradas en el firmware son:

| Pin MP34DT05TR | Pin RP2040 (GPIO) | Descripción |
|---|---|---|
| CLK | GPIO 5 | Reloj del PDM |
| DIN | GPIO 4 | Datos de entrada del PDM |
| GND | GND | Tierra |
| VCC | 3.3V | Alimentación |

**Nota**: Puedes cambiar estos pines editando las constantes `kPdmDinPin` y `kPdmClkPin` en el archivo `.ino`.

---

## 🔌 Configuración del Firmware

### Parámetros de Captura

El firmware está configurado con los siguientes parámetros:

```cpp
static const int kSampleRate = 8000;        // Frecuencia de muestreo: 8 kHz
static const int kChannels = 1;             // 1 canal (mono)
static const size_t kSampleBufferCount = 256; // Tamaño del buffer
static const int kPdmDinPin = 4;            // Pin de datos
static const int kPdmClkPin = 5;            // Pin de reloj
```

### Formato de Datos

- **Formato**: PCM 16-bit signed (int16_t)
- **Velocidad de Transmisión Serial**: 115200 bps
- **Buffer de Datos**: Se envían bloques de hasta 256 muestras en formato binario
- **Tamaño de Cada Muestra**: 2 bytes (16 bits)

---

## 🚀 Instalación y Configuración

### 1️⃣ Requisitos de Desarrollo

#### Para el Firmware (Arduino)

- **Arduino IDE** o **PlatformIO** (recomendado para RP2040)
- **Tablero**: Arduino-Pico (Board Support Package para RP2040)
  - En Arduino IDE: `Boards Manager → Search "pico" → Install "Arduino Mbed OS RP2040 Boards"` o `"Raspberry Pi Pico/RP2040"`

#### Para el Script Python

```bash
# Python 3.8 o superior
python --version

# Instalar dependencias
pip install pyserial websockets
```

### 2️⃣ Instalación del Firmware

1. **Conectar la DualMCUOne por USB** a tu computadora
2. **Abrir el archivo** `MainCode_DualMCUOne_Socket_audio.ino` en Arduino IDE o PlatformIO
3. **Seleccionar la placa correcta**:
   - Boards: `Raspberry Pi Pico` o similar
   - Port: Seleccionar el puerto COM donde está conectada la placa
4. **Compilar y subir** el firmware a la placa
5. **Abrir el Monitor Serial** (115200 bps) para verificar:
   - Mensaje: `hello PDM microphone (Arduino)` = ✅ Éxito
   - Error: `PDM microphone initialization failed!` = ❌ Revisar conexiones

### 3️⃣ Instalación del Script Python

1. **Instalar dependencias**:
   ```bash
   pip install pyserial websockets
   ```

2. **Verificar puerto serial disponible** (Windows/PowerShell):
   ```powershell
   Get-WmiObject Win32_SerialPort
   ```
   
   O usar el menú interactivo del script (se ejecuta automáticamente)

3. **Ejecutar el servidor WebSocket**:
   ```bash
   python audio.py
   ```

   El script te mostrará un menú de puertos COM disponibles. Selecciona el correcto.

---

## 📡 Funcionamiento del Sistema

### Firmware Arduino (Lado Microcontrolador)

1. **Inicialización**:
   - Configura comunicación Serial a 115200 bps
   - Establece los pines PDM (DIN=GPIO4, CLK=GPIO5)
   - Inicia el PDM a 8 kHz, 1 canal
   - Registra callback `onPdmData()` para interrupciones

2. **Captura de Audio**:
   - El callback `onPdmData()` se activa cuando hay datos PDM disponibles
   - Lee las muestras del buffer PDM (máximo 256 muestras)
   - Almacena en `sampleBuffer` como int16_t

3. **Transmisión**:
   - En cada ciclo del `loop()`, si hay muestras disponibles, las envía en **formato binario** al puerto serial
   - Usa `Serial.write()` para enviar bytes crudos (no imprime texto)
   - Los datos se envían en bloques de hasta 512 bytes (256 muestras × 2 bytes)

### Script Python (Lado Server)

1. **Selección de Puerto**:
   - Detecta automáticamente todos los puertos COM disponibles
   - Muestra un menú interactivo para elegir
   - Se conecta al puerto con configuración 115200 bps, sin timeout de lectura

2. **Servidor WebSocket**:
   - Escucha en `ws://localhost:8765`
   - Acepta conexiones de clientes web
   - Mantiene la conexión abierta mientras hay datos seriales

3. **Flujo de Datos**:
   - Lee continuamente del puerto serial cada 5 ms
   - Los datos se leen sin bloqueo (`timeout=0`)
   - Retransmite inmediatamente todos los bytes recibidos al cliente WebSocket
   - Si el cliente se desconecta, espera una nueva conexión

---

## 🎯 Uso Paso a Paso

### Paso 1: Preparar el Hardware
```
1. Conectar el micrófono MP34DT05TR a la DualMCUOne
   - CLK → GPIO 5
   - DIN → GPIO 4
   - GND → GND
   - VCC → 3.3V

2. Conectar la DualMCUOne por USB a tu PC
```

### Paso 2: Subir el Firmware
```
1. Abrir MainCode_DualMCUOne_Socket_audio.ino en Arduino IDE
2. Seleccionar la placa: Raspberry Pi Pico
3. Seleccionar el puerto COM correcto
4. Click en "Upload" (o Sketch → Upload)
5. Esperar a que termine la carga
6. Abrir Serial Monitor (115200 bps)
7. Verificar mensaje: "hello PDM microphone (Arduino)"
```

### Paso 3: Ejecutar el Script Python
```bash
# En una terminal (PowerShell, CMD, o bash)
cd "path/to/MainCode_DualMCUOne_Socket_audio"
python audio.py

# Salida esperada:
# --- PUERTOS COM DISPONIBLES ---
# [0] COM3 - USB Serial Device
# Elige el número del puerto que deseas usar (ej. 0): 0
# ✅ Puerto seleccionado: COM3
# 
# 🚀 Iniciando servidor WebSocket en ws://localhost:8765
# 🎧 Ve a tu navegador web y presiona LISTEN para empezar...
```

### Paso 4: Conectar el Frontend Web
```javascript
// Desde tu frontend web (JavaScript)
const ws = new WebSocket('ws://localhost:8765');

ws.onopen = () => {
    console.log('Conectado al servidor de audio');
};

ws.onmessage = (event) => {
    // event.data contiene los bytes de audio en tiempo real
    // Procesar datos PCM 16-bit, 8 kHz mono
    const audioBuffer = new Int16Array(event.data);
    // ... procesar con Web Audio API, etc.
};

ws.onerror = (error) => {
    console.error('Error WebSocket:', error);
};
```

---

## 🔍 Formato de Datos de Audio

### Especificaciones

- **Tipo**: PCM (Pulse Code Modulation)
- **Profundidad**: 16 bits por muestra (signed)
- **Canales**: 1 (Mono)
- **Sample Rate**: 8000 Hz (8 kHz)
- **Bitrate**: 128 kbps (8000 × 16 × 1)
- **Formato de Transmisión**: Binario (bytes crudos)

### Ejemplo de Interpretación en JavaScript

```javascript
ws.onmessage = (event) => {
    // Convertir bytes a Int16Array (little-endian)
    const audioData = new Int16Array(event.data);
    
    console.log(`Recibidas ${audioData.length} muestras`);
    console.log(`Primera muestra: ${audioData[0]}`);
    
    // Los valores están entre -32768 y +32767
    // Normalizar a rango [-1, 1] para visualización
    const normalized = audioData.map(s => s / 32768);
};
```

---

## ⚙️ Personalización

### Cambiar la Frecuencia de Muestreo

En `MainCode_DualMCUOne_Socket_audio.ino`:

```cpp
static const int kSampleRate = 8000;  // Cambiar a 16000 para 16 kHz, etc.
```

**Opciones comunes para RP2040**:
- 8000 Hz (teléfono/voz)
- 16000 Hz (mejor calidad de voz)
- 22050 Hz, 44100 Hz (audio de mayor calidad)

### Cambiar los Pines PDM

En `MainCode_DualMCUOne_Socket_audio.ino`:

```cpp
static const int kPdmDinPin = 4;   // Cambiar a otro GPIO disponible
static const int kPdmClkPin = 5;   // Cambiar a otro GPIO disponible
```

Luego en `setup()`, los pines se configuran automáticamente antes de inicializar el PDM.

### Cambiar el Puerto WebSocket

En `audio.py`:

```python
WS_PORT = 8765  # Cambiar a otro puerto (ej. 8000, 8080, etc.)
```

---

## 🐛 Resolución de Problemas

### ❌ "PDM microphone initialization failed!"

**Causa**: Problema en la conexión del micrófono o pines incorrectos

**Solución**:
1. Verificar conexiones del micrófono (CLK en GPIO5, DIN en GPIO4)
2. Revisar que GND y 3.3V estén correctamente conectados
3. Probar con un multímetro que haya continuidad en las líneas
4. Si usas otros pines, editar `kPdmDinPin` y `kPdmClkPin`

### ⚠️ No aparece "hello PDM microphone (Arduino)" en Serial Monitor

**Causa**: Firmware no se subió correctamente

**Solución**:
1. Verificar que el puerto COM sea correcto
2. Seleccionar la placa correcta (Raspberry Pi Pico)
3. Reintentar upload del firmware
4. Presionar botón RESET en la placa DualMCUOne

### ❌ Error "Port in use" en Python

**Causa**: El puerto serial ya está siendo usado

**Solución**:
1. Cerrar Serial Monitor de Arduino IDE
2. Si sigue sin funcionar: `taskkill /IM python.exe` (Windows)
3. Desconectar y reconectar la placa USB

### ⚠️ El frontend no recibe datos

**Causa**: El servidor WebSocket no está ejecutándose o hay firewall

**Solución**:
1. Verificar que `audio.py` está corriendo sin errores
2. Probar en `localhost` primero (no desde otra máquina)
3. Desactivar temporalmente firewall de Windows
4. Revisar que el frontend se conecte a `ws://localhost:8765`

### 🔇 Datos seriales llegando pero audio vacío

**Causa**: El micrófono no capta sonido (silenciado, desconectado, etc.)

**Solución**:
1. Verificar que el micrófono esté correctamente alimentado (3.3V)
2. Probar con ruido fuerte cerca del micrófono
3. Revisar la hoja de datos del MP34DT05TR para requisitos de alimentación
4. Verificar que el DOUT (data out) del micrófono esté en el DIN correcto

---

## 📚 Referencias Útiles

### Documentación del Hardware

- [RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)
- [MP34DT05TR Datasheet](https://www.st.com/resource/en/datasheet/mp34dt05-a.pdf)
- [Arduino-Pico GitHub](https://github.com/earlephilhower/arduino-pico)

### Librerías Utilizadas

- **PDM.h**: Incluida en Arduino-Pico para captura de audio PDM
- **pyserial**: Para lectura del puerto serial en Python
- **websockets**: Librería asíncrona de WebSocket para Python

### Estándares de Audio

- **PCM**: Pulse Code Modulation (estándar de audio digital)
- **PDM**: Pulse Density Modulation (codificación de 1 bit de alta frecuencia)
- **Conversión PDM a PCM**: Realizada internamente por el PDM.h

---

## 📝 Notas Importantes

1. **Latencia**: El sistema introduce una latencia mínima (~5-10 ms) del micrófono al navegador
2. **Ancho de Banda**: Con 8 kHz mono, se transmiten ~128 kbps, muy eficiente
3. **Múltiples Frontends**: El servidor WebSocket actual solo maneja 1 conexión a la vez
4. **Escalabilidad**: Para múltiples clientes, modificar `audio.py` para usar un broadcast pattern

---


## 📄 Licencia

Este proyecto es parte del TestBench UE0011 PDM MP34DT05TR Microphone.

---

**Última actualización**: Junio 2026

Para más información o soporte, consulta la documentación de Arduino-Pico y la hoja de datos del MP34DT05TR.
