#!/bin/bash

# --- CONFIGURACIÓN ---
# El archivo debe ser extensión .uf2 (Arduino lo genera al compilar)
FIRMWARE_PATH="blink_rp2040.ino.uf2" 

RP2040_DRIVE=""

# --- FUNCIÓN PARA DETECTAR RP2040 ---
detectar_rp2040() {
    echo "Buscando RP2040 en modo Bootloader (RPI-RP2)..."
    RP2040_DRIVE=""
    
    # En Git Bash, las unidades de Windows se montan como /d, /e, /f, etc.
    # Recorremos letras probables (d..z)
    for drive in {d..z}; do
        # Buscamos el archivo INFO_UF2.TXT que siempre existe en el RP2040 virgen
        if [ -f "/$drive/INFO_UF2.TXT" ]; then
            RP2040_DRIVE="/$drive"
            # Intentamos leer el modelo exacto dentro del archivo txt
            MODEL=$(cat /$drive/INFO_UF2.TXT | grep "Model" | cut -d: -f2)
            echo "✅ ¡Encontrado! Unidad: ${drive^^}: (Ruta: $RP2040_DRIVE)"
            echo "   Modelo detectado:$MODEL"
            return
        fi
    done

    if [ -z "$RP2040_DRIVE" ]; then
        echo "⚠️  NO SE DETECTÓ NINGÚN RP2040."
        echo "   Asegúrate de conectar el USB manteniendo presionado el botón BOOTSEL."
    fi
}

# Detección inicial
detectar_rp2040
sleep 1

# --- MENÚ ---
while true; do
    echo "========================================"
    echo "      HERRAMIENTA FLASH RP2040"
    echo "========================================"
    echo "Firmware: $FIRMWARE_PATH"
    if [ -z "$RP2040_DRIVE" ]; then
        echo "Estado: 🔴 NO CONECTADO (Modo BOOTSEL)"
    else
        echo "Estado: 🟢 LISTO en $RP2040_DRIVE"
    fi
    echo "========================================"
    echo "1. Redetectar (Buscar unidad RPI-RP2)"
    echo "2. Flashear Firmware (Copiar .uf2)"
    echo "3. Salir"
    echo "========================================"
    read -p "Selecciona una opción: " opcion

    case $opcion in
        1)
            detectar_rp2040
            read -p "Enter para continuar..."
            ;;
        2)
            if [ -z "$RP2040_DRIVE" ]; then
                echo "❌ ERROR: No hay RP2040 detectado. Conéctalo con BOOTSEL presionado."
            elif [ ! -f "$FIRMWARE_PATH" ]; then
                echo "❌ ERROR: No encuentro el archivo $FIRMWARE_PATH en esta carpeta."
            else
                echo "Copiando firmware a $RP2040_DRIVE..."
                
                # Comando simple de copia
                cp "$FIRMWARE_PATH" "$RP2040_DRIVE/"
                
                if [ $? -eq 0 ]; then
                    echo "✅ ¡COPIA EXITOSA!"
                    echo "El RP2040 se reiniciará automáticamente y ejecutará el código."
                    # Al reiniciarse, el disco RPI-RP2 desaparece solo.
                    RP2040_DRIVE="" 
                else
                    echo "❌ Falló la copia."
                fi
            fi
            read -p "Enter para continuar..."
            ;;
        3)
            echo "Saliendo..."
            exit 0
            ;;
        *)
            echo "Opción no válida."
            ;;
    esac
done