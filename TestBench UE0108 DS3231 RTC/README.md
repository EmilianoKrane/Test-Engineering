# TestBench UE0108 DS3231 RTC

Este repositorio contiene el firmware de prueba para el módulo UE0108 Test DevLab: I2C DS3231 RTC Module.

## Descripción

El firmware inicializa el RTC DS3231 mediante el bus I2C, verifica la hora y permite ajustar la hora por comandos JSON recibidos por UART2.

## Conexiones

- `GPIO6` - SDA I2C
- `GPIO7` - SCL I2C
- `GPIO4` - Botón de arranque
- `GPIO15` - RX2 (UART2)
- `GPIO19` - TX2 (UART2)

El módulo RTC debe conectarse al conector JST/Qwiic de la Pulsar C6 para comunicarse vía I2C.

## Archivos principales

- `MainCode_JSON_UE0108_RTC_DS3231/MainCode_JSON_UE0108_RTC_DS3231.ino`: firmware principal.

## Comandos JSON soportados

- `{"Function":"ping"}`
- `{"Function":"init"}`
- `{"Function":"checkHour"}`
- `{"Function":"setHour", "datetime":"YYYY-MM-DD HH:MM:SS"}`

## Nota

El código está diseñado para ejecutarse en el testbench y recibir las instrucciones JSON a través del UART2. Si se desea usar otro puerto serie, debe modificarse el objeto `PagWeb` en el firmware.
