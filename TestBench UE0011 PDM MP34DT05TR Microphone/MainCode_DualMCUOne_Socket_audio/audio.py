import asyncio
import websockets
import serial
import serial.tools.list_ports

# Configuración
BAUD_RATE = 115200  # Asegúrate de que coincida con el de tu Arduino
WS_PORT = 8765

def select_com_port():
    """Busca los puertos y muestra un menú interactivo en la terminal."""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("❌ No se detectó ningún puerto COM.")
        return None
    
    print("\n--- PUERTOS COM DISPONIBLES ---")
    for i, p in enumerate(ports):
        print(f"[{i}] {p.device} - {p.description}")
        
    seleccion = input("\nElige el número del puerto que deseas usar (ej. 0): ")
    
    try:
        idx = int(seleccion)
        puerto_elegido = ports[idx].device
        print(f"✅ Puerto seleccionado: {puerto_elegido}\n")
        return puerto_elegido
    except (ValueError, IndexError):
        print(f"⚠️ Selección no válida. Usando por defecto: {ports[0].device}\n")
        return ports[0].device

async def audio_stream_handler(websocket, port):
    """Maneja la conexión de un cliente WebSocket y le envía los datos seriales."""
    print(f"🔌 Frontend conectado. Abriendo {port} a {BAUD_RATE} bps...")
    
    try:
        # timeout=0 hace que la lectura no bloquee el hilo asíncrono
        with serial.Serial(port, BAUD_RATE, timeout=0) as ser:
            ser.reset_input_buffer()
            
            while True:
                # Leer todos los bytes disponibles en el buffer serial
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    # Enviar los bytes crudos directamente al frontend web
                    await websocket.send(data)
                
                # Ceder el control al event loop por 5ms para no saturar la CPU
                await asyncio.sleep(0.005)
                
    except websockets.exceptions.ConnectionClosed:
        print("🛑 Frontend desconectado. Esperando nueva conexión...")
    except serial.SerialException as e:
        print(f"❌ Error al abrir o leer el puerto serial: {e}")
    except Exception as e:
        print(f"⚠️ Error inesperado: {e}")

async def main():
    # 1. Preguntar por el puerto antes de iniciar el servidor
    selected_port = select_com_port()
    if not selected_port:
        return

    # 2. Crear un "wrapper" para pasar el puerto al handler del WebSocket
    async def handler(ws):
        await audio_stream_handler(ws, selected_port)

    # 3. Levantar el servidor
    print(f"🚀 Iniciando servidor WebSocket en ws://localhost:{WS_PORT}")
    print("🎧 Ve a tu navegador web y presiona LISTEN para empezar...")
    
    async with websockets.serve(handler, "localhost", WS_PORT):
        await asyncio.Future()  # Mantiene el servidor corriendo indefinidamente

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nServidor detenido por el usuario.")