#!/bin/bash

# --- CONFIGURACIÓN ---
#FIRMWARE_PATH="blink_rp2040.ino.uf2" 
FIRMWARE_PATH="blink_pulsarRP2350A.ino.uf2" 

# Lista para guardar las unidades encontradas (ej: "/d /e /f")
DRIVES_LIST=""

# --- FUNCIÓN PARA DETECTAR MÚLTIPLES RP2040 ---
detectar_rp2040() {
    echo "Buscando múltiples RP2350 (RPI-RP2)..."
    DRIVES_LIST=""
    COUNT=0
    
    # Recorremos letras probables (d..z)
    for drive in {d..z}; do
        # Buscamos la firma del RP2040
        if [ -f "/$drive/INFO_UF2.TXT" ]; then
            # Añadimos la unidad a la lista
            DRIVES_LIST="$DRIVES_LIST /$drive"
            ((COUNT++))
        fi
    done

    if [ -z "$DRIVES_LIST" ]; then
        echo "⚠️  NO SE DETECTARON PLACAS."
        echo "   Recuerda conectar usando BOOTSEL."
    else
        echo "✅ Se encontraron $COUNT dispositivos en:$DRIVES_LIST"
    fi
}

# Detección inicial
detectar_rp2040
sleep 1

# --- MENÚ ---
while true; do
    echo "========================================"
    echo "   HERRAMIENTA FLASH RP2040 MULTIPLE"
    echo "========================================"
    echo "Firmware: $FIRMWARE_PATH"
    if [ -z "$DRIVES_LIST" ]; then
        echo "Placas detectadas: NINGUNA"
    else
        echo "Placas detectadas:$DRIVES_LIST"
    fi
    echo "========================================"
    echo "1. Redetectar (Escanear unidades)"
    echo "2. Flashear TODAS (Copia masiva)"
    echo "3. Salir"
    echo "========================================"
    read -p "Selecciona una opción: " opcion

    case $opcion in
        1)
            detectar_rp2040
            read -p "Enter para continuar..."
            ;;
        2)
            if [ -z "$DRIVES_LIST" ]; then
                echo "❌ ERROR: No hay placas conectadas."
            elif [ ! -f "$FIRMWARE_PATH" ]; then
                echo "❌ ERROR: No encuentro el archivo $FIRMWARE_PATH"
            else
                echo "Iniciando copia masiva..."
                
                # Bucle para copiar a cada unidad detectada
                for DEST in $DRIVES_LIST; do
                    (
                        echo ">>> Copiando a unidad ${DEST^^} ..."
                        cp "$FIRMWARE_PATH" "$DEST/"
                        
                        if [ $? -eq 0 ]; then
                            echo ">>> ✅ ${DEST^^} Completado (Se reiniciará)"
                        else
                            echo ">>> ❌ Error al copiar en ${DEST^^}"
                        fi
                    ) & # El '&' hace que se copien todos en paralelo
                done
                
                wait # Esperamos a que terminen todas las copias
                echo "--- Proceso finalizado ---"
                
                # Limpiamos la lista porque al reiniciarse se desconectan
                DRIVES_LIST=""
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