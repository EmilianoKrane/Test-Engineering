@echo off
cls
title Flashear ATmega328P con USBasp

set AVRDUDE="C:\Users\emili\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino17\bin\avrdude.exe"
set CONF="C:\Users\emili\AppData\Local\Arduino15\packages\arduino\tools\avrdude\6.3.0-arduino17\etc\avrdude.conf"

echo.
echo ==========================================
echo   PROGRAMANDO ATmega328P
echo ==========================================
echo.

REM ===== Validar archivo =====
if "%1"=="" (
    color 0C
    echo ERROR: No indicaste un archivo .hex
    echo Uso: flash328p archivo.hex
    goto end
)

if not exist "%1" (
    color 0C
    echo ERROR: El archivo "%1" no existe
    goto end
)

echo Archivo: %1
echo Programador: USBasp
echo.

REM ===== Ejecutar programación =====
color 07
%AVRDUDE% -C%CONF% -c usbasp -p m328p -U flash:w:%1:i
set RESULT=%ERRORLEVEL%

echo.
echo ==========================================

REM ===== Verificar resultado =====
if %RESULT%==0 (
    color 0A
    echo   PROGRAMACION COMPLETADA CON EXITO
    echo ==========================================
    goto end
) else (
    color 0C
    echo   ERROR AL PROGRAMAR EL ATmega328P
    echo   Codigo de error: %RESULT%
    echo ==========================================
)

:end
echo.
pause
