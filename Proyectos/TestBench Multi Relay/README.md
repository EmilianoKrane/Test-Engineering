# TestBench Multi Relay

Este proyecto implementa un banco de pruebas para un multiplexador de relevadores utilizando comunicación I2C entre un maestro y un esclavo.

## Estructura del Proyecto

- **Main Slave/**: Contiene el firmware del esclavo (ESP32C6) que controla el multiplexador de relevadores.
- **MainCode_TestBench_Master_I2C/**: Contiene el firmware del maestro (Pulsar C6) que se comunica con el esclavo vía I2C.

## Descripción General

El sistema consta de dos componentes principales:
- **Esclavo**: ESP32C6 configurado como dispositivo esclavo I2C, responsable de activar/desactivar relevadores individuales.
- **Maestro**: Pulsar C6 configurado como maestro I2C, que envía comandos al esclavo y recibe respuestas.

La comunicación se realiza a través del protocolo I2C, permitiendo controlar hasta 16 canales de relevadores.

## Funcionalidades

- Activación individual de relevadores (canales 1-16)
- Barrido automático de todos los relevadores
- Modo de suspensión
- Comunicación JSON para integración con interfaces web
- Depuración a través de serial y UART

## Requisitos

- ESP32C6 para el esclavo
- Pulsar C6 para el maestro
- Arduino IDE con las librerías necesarias (ArduinoJson, Wire)
- Conexión I2C entre maestro y esclavo