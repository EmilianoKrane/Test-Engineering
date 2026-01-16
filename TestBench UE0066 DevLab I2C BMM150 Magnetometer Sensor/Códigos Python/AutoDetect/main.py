# ⚠️ IMPORTANT HARDWARE NOTICE – BMM150 I2C/SPI interface
#
# The BMM150 sensor cannot use I2C and SPI simultaneously, as the SDO and CS pins are shared
# between both interfaces and behave differently depending on wiring.
#
# 🔸 If CS is tied to GND, the sensor enters SPI mode and will not respond to I2C commands.
# 🔸 If CS is high or floating (with pull-up), the sensor is in I2C mode.
# 🔸 Having both I2C and SPI buses physically connected at the same time may cause signal interference,
#     especially if SPI pins (MOSI, MISO, SCK, CS) are active while the sensor is in I2C mode.
#
# ✅ Recommended best practices:
# - Disconnect the SPI bus when using I2C (or add 1kΩ series resistors to reduce interference).
# - Use jumpers or DIP switches to toggle between I2C and SPI mode.
# - Never connect both I2C and SPI to the same sensor module simultaneously unless using isolation.
#
# This is especially important when using auto-detection between I2C and SPI.

import machine
from machine import I2C, SPI, Pin
import time
from bmm150_auto_interface import BMM150_Auto
import ujson

# Pines de UART con la PagWeb
TX_PIN2 = 5
RX_PIN2 = 4

# UART para comunicar con la PagWeb
uart2 = machine.UART(2, baudrate=115200, tx=machine.Pin(TX_PIN2, machine.Pin.OUT), rx=machine.Pin(RX_PIN2, machine.Pin.IN))

# I2C on GPIO 4 (SDA) and 5 (SCL)
i2c = I2C(0, sda=Pin(6), scl=Pin(7), freq=100_000)

# SPI on GPIO 18 (SCK), 19 (MOSI), 16 (MISO) and CS on GPIO 17
spi = SPI(1, baudrate=1_000_000, polarity=0, phase=0, sck=Pin(20), mosi=Pin(19), miso=Pin(2))
cs = Pin(18, Pin.OUT)

sensor = BMM150_Auto(i2c, spi, cs)

def handle_command(cmd):
    global sample_time

    # {"cmd":"read_mag"}
    if cmd["cmd"] == "read_mag":

        x, y, z, resp, iface, addr = sensor.read_raw()
        response = {
            "resp": resp,
            "iface": iface,
            "addr": hex(addr),
            "x": x,
            "y": y,
            "z": z
        }
        uart2.write(ujson.dumps(response) + "\n")


    # {"cmd":"ping"}
    elif cmd["cmd"] == "ping":
        uart2.write(ujson.dumps({"resp": "pong"}) + "\n")

    else:
        uart2.write(ujson.dumps({
            "resp": "error",
            "msg": "cmd desconocido"
        }) + "\n")


def pagweb_uart():
    if not uart2.any():
        return

    line = uart2.readline()
    print("UART RX:", line)
    
    if not line:
        print("No hay información")
        return

    line = line.strip()

    if not (line.startswith(b'{') and line.endswith(b'}')):
        return

    try:
        cmd = ujson.loads(line.decode("utf-8"))
        print("cmd:", cmd)
        handle_command(cmd)
    except Exception:
        uart2.write(ujson.dumps({
            "resp": "error",
            "msg": "JSON inválido"
        }) + "\n")

'''
while True: 
    pagweb_uart()
    time.sleep(1)
'''

while True:
    x, y, z, resp, iface, addr = sensor.read_raw()
    response = {
        "resp": resp,
        "iface": iface,
        "addr": addr,
        "x": x,
        "y": y,
        "z": z
    }
    uart2.write(ujson.dumps(response) + "\n")
    time.sleep(2)
    
    
    
    