#!/bin/bash

# --- CONFIGURACIÓN ---
FIRMWARE_PATH="blink_pulsarRP2350A.ino.uf2" 

# Variable global para saber cuántos esperamos
EXPECTED_COUNT=0

# --- PASO 1: DETECTAR COM Y REINICIAR ---
resetear_picos() {
    echo "---------------------------------------------------"
    echo "PASO 1: Buscando RP20350 en modo Serial (COM)..."
    
    PORTS_TO_RESET=$(python -c "import serial.tools.list_ports; ports = [p.device for p in serial.tools.list_ports.comports() if '2E8A' in p.hwid]; print(' '.join(ports))")

    if [ -z "$PORTS_TO_RESET" ]; then
        echo "⚠️  No detecté ningún Pico como puerto COM."
        EXPECTED_COUNT=0
    else
        # Contamos cuántos dispositivos vamos a reiniciar
        EXPECTED_COUNT=$(echo $PORTS_TO_RESET | wc -w)
        
        echo "🔄 Detectados $EXPECTED_COUNT dispositivos: $PORTS_TO_RESET"
        echo "   Enviando señal mágica (1200 baud)..."
        
        for PORT in $PORTS_TO_RESET; do
            python -c "import serial; import time; ser = serial.Serial('$PORT', 1200); ser.close()" 2>/dev/null
            echo "   👉 Señal enviada a $PORT"
        done
        
        echo "⏳ Esperando a que Windows monte las unidades..."
    fi
}

# --- PASO 2: FLASHEAR UNIDADES (CON ESPERA INTELIGENTE) ---
flashear_unidades() {
    echo "---------------------------------------------------"
    echo "PASO 2: Buscando unidades RPI-RP2..."
    
    DRIVES_LIST=""
    FOUND_COUNT=0
    
    # BUCLE DE ESPERA (MAX 15 INTENTOS / 15 SEGUNDOS)
    for attempt in {1..20}; do
        # Reiniciamos la lista en cada intento
        TEMP_LIST=""
        TEMP_COUNT=0
        
        # Escaneamos de D a Z
        for drive in {d..z}; do
            if [ -f "/$drive/INFO_UF2.TXT" ]; then
                TEMP_LIST="$TEMP_LIST /$drive"
                ((TEMP_COUNT++))
            fi
        done
        
        # Actualizamos las variables globales
        DRIVES_LIST=$TEMP_LIST
        FOUND_COUNT=$TEMP_COUNT
        
        # LOGICA DE SALIDA:
        # Si no esperamos ninguno en específico (Opción 2 manual), basta con encontrar 1.
        # Si venimos de un Reset (Opción 1), esperamos encontrar TODOS.
        if [ "$EXPECTED_COUNT" -gt 0 ]; then
             echo "   [Intento $attempt/20] Encontrados: $FOUND_COUNT de $EXPECTED_COUNT esperados..."
             if [ "$FOUND_COUNT" -ge "$EXPECTED_COUNT" ]; then
                 break # ¡Ya están todos! Salimos del bucle de espera
             fi
        else
             # Modo manual: si encontramos algo, salimos, si no, esperamos un poco más
             if [ "$FOUND_COUNT" -gt 0 ]; then
                 break
             else
                 echo "   [Intento $attempt/15] Buscando..."
             fi
        fi
        
        sleep 1 # Esperar 1 segundo antes de volver a escanear
    done

    # --- FASE DE COPIA ---
    if [ -z "$DRIVES_LIST" ]; then
        echo "❌ TIEMPO AGOTADO: No se detectaron unidades RPI-RP2."
    else
        echo "✅ Unidades listas ($FOUND_COUNT): $DRIVES_LIST"
        echo "🚀 Iniciando carga masiva..."
        
        for DEST in $DRIVES_LIST; do
            (
                echo "   >>> Copiando a ${DEST^^} ..."
                cp "$FIRMWARE_PATH" "$DEST/"
                if [ $? -eq 0 ]; then
                    echo "   >>> ✅ ${DEST^^} Listo y reiniciando."
                else
                    echo "   >>> ❌ Error en ${DEST^^}"
                fi
            ) &
        done
        wait
        echo "🎉 ¡Proceso terminado!"
        # Reseteamos el contador esperado para la próxima vez
        EXPECTED_COUNT=0
    fi
}

# --- MENÚ PRINCIPAL ---
while true; do
    echo "========================================"
    echo "   AUTOMATIZADOR RP2040 (SIN BOTONES)"
    echo "========================================"
    echo "Archivo: $FIRMWARE_PATH"
    echo "1. Ejecutar CICLO COMPLETO (Reset + Flash)"
    echo "2. Solo Flashear (Si ya usaste el botón)"
    echo "3. Salir"
    echo "========================================"
    read -p "Opción: " opcion

    case $opcion in
        1)
            resetear_picos
            flashear_unidades
            read -p "Enter para continuar..."
            ;;
        2)
            EXPECTED_COUNT=0 # En modo manual no sabemos cuántos esperar
            flashear_unidades
            read -p "Enter para continuar..."
            ;;
        3)
            exit 0
            ;;
        *)
            echo "Opción no válida"
            ;;
    esac
done