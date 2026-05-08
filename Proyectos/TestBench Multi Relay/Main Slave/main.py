from machine import Pin
from neopixel import NeoPixel
import i2c_slave
import time

# --- Configuración de pines del 74HC4067 ---
S0 = Pin(0, Pin.OUT)
S1 = Pin(1, Pin.OUT)
S2 = Pin(3, Pin.OUT)
S3 = Pin(4, Pin.OUT)
select_pins = [S0, S1, S2, S3]

# --- Pin EN del MUX ---
EN_MUX = Pin(15, Pin.OUT)
EN_MUX.value(1)  # Inicia deshabilitado (HIGH)

# --- NeoPixel ---
np = NeoPixel(Pin(8), 1)

# --- Lectura de DIP switch (3 bits) ---
dip_pins = [
    Pin(21, Pin.IN, Pin.PULL_DOWN),
    Pin(20, Pin.IN, Pin.PULL_DOWN),
    Pin(18, Pin.IN, Pin.PULL_DOWN)
]

def leer_dip_switch():
    val = 0
    for i, pin in enumerate(dip_pins):
        val |= (pin.value() << i)
    return 0x40 + val  # Rango 0x40–0x47

SLAVE_ADDR = leer_dip_switch()
print(f"Dirección I2C configurada desde DIP switch: 0x{SLAVE_ADDR:X}")

# --- Inicializar I2C esclavo ---
slave = i2c_slave.I2CSlave(0, SLAVE_ADDR, 23, 22)
slave.init()

# --- Función para activar canal del mux ---
def set_channel(channel):
    EN_MUX.value(0)  # Habilitar mux
    for i, pin in enumerate(select_pins):
        pin.value((channel >> i) & 1)
    # Confirmación visual
    if channel % 2 == 0:
        np[0] = (0, 50, 0)
    else:
        np[0] = (0, 0, 50)
    np.write()

# --- Función para apagar NeoPixel y mux ---
def apagar():
    EN_MUX.value(1)  # Deshabilitar mux
    for pin in select_pins:
        pin.value(0)
    np[0] = (0, 0, 0)
    np.write()

# --- Función para barrer todos los canales ---
def barrido():
    EN_MUX.value(0)  # Habilitar mux
    for ch in range(16):
        set_channel(ch)
        time.sleep(0.2)
    apagar()

print(f"Esclavo 0x{SLAVE_ADDR:X} listo con mux + NeoPixel")

# --- Loop principal ---
while True:
    cmd = slave.read_command(1000)
    if cmd is not None:
        if cmd == 0xFE:
            apagar()
            print("Modo reposo: NeoPixel apagado y mux deshabilitado")
            slave.set_response(0xFE)

        elif cmd == 0xFF:
            barrido()
            print("Barrido completado")
            slave.set_response(0xFF)

        else:
            channel = cmd & 0x0F
            set_channel(channel)
            print("Canal activo:", channel)
            slave.set_response(channel)

        slave.send_response()






