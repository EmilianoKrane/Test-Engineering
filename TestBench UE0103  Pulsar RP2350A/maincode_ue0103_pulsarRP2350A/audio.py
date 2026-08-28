import asyncio
import websockets
import serial
import serial.tools.list_ports

# Configuración
BAUD_RATE = 230400  # Asegúrate de que coincida con el de tu Arduino
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
    print(f"🔌 WebSocket conectado. Abriendo puente bidireccional en {port} a {BAUD_RATE} bps...")
    
    try:
        with serial.Serial(port, BAUD_RATE, timeout=0) as ser:
            print("✅ ÉXITO: Puerto serial abierto por Python.")
            ser.reset_input_buffer()
            
            # TAREA 1: Escuchar JSON de React y enviarlo a la Pulsar RP2350
            async def frontend_to_micro():
                async for message in websocket:
                    print(f"📩 Comando recibido de React. Inyectando al {port}: {message.strip()}")
                    if isinstance(message, str):
                        ser.write(message.encode('utf-8'))
                    else:
                        ser.write(message)
                    ser.flush()

            # TAREA 2: Escuchar Audio de la Pulsar y enviarlo a React
            async def micro_to_frontend():
                while True:
                    if ser.in_waiting > 0:
                        data = ser.read(ser.in_waiting)
                        await websocket.send(data)
                    await asyncio.sleep(0.005)

            # Ejecutar ambas tareas en paralelo
            task1 = asyncio.create_task(frontend_to_micro())
            task2 = asyncio.create_task(micro_to_frontend())

            # Mantener vivas hasta que el usuario cierre la conexión
            done, pending = await asyncio.wait(
                [task1, task2],
                return_when=asyncio.FIRST_COMPLETED
            )
            
            # Limpiar si se desconecta
            for task in pending:
                task.cancel()
                
    except websockets.exceptions.ConnectionClosed:
        print("🛑 Frontend desconectado. Cerrando puerto COM.")
    except serial.SerialException as e:
        print(f"\n❌ ERROR DE PUERTO: {e}")
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