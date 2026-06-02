#!/usr/bin/env python3
"""
Live serial viewer for PDM sample streams.

- Reads signed integer samples from serial (one value per line or CSV lines)
- Keeps a rolling window of recent samples
- Clears and redraws the terminal on every refresh
"""

from __future__ import annotations
import argparse
import math
import statistics
import time
from collections import deque

import serial
import serial.tools.list_ports


def detect_default_port() -> str | None:
    ports = serial.tools.list_ports.comports()
    
    if not ports:
        return None
        
    # Si solo hay uno, lo toma automáticamente
    if len(ports) == 1:
        print(f"Detectado automáticamente: {ports[0].device}")
        return ports[0].device

    # Si hay varios, te muestra una lista para elegir
    print("Se detectaron múltiples puertos COM:")
    for i, port in enumerate(ports):
        print(f"[{i}] {port.device} - {port.description}")
        
    seleccion = input("Escribe el número del puerto que deseas usar (ej. 0, 1): ")
    
    try:
        idx = int(seleccion)
        return ports[idx].device
    except (ValueError, IndexError):
        print("Selección no válida. Usando el primer puerto por defecto.")
        return ports[0].device

def parse_samples(raw_line: str) -> list[int]:
    raw_line = raw_line.strip()
    if not raw_line:
        return []

    parts = raw_line.split(",")
    values: list[int] = []
    for part in parts:
        token = part.strip()
        if not token:
            continue
        try:
            values.append(int(token))
        except ValueError:
            # Ignore non-numeric lines (boot/status logs)
            continue
    return values


def render_screen(
    port: str,
    baud: int,
    sample_rate: int,
    window: deque[int],
    total: int,
    dropped: int,
) -> None:
    print("\033[2J\033[H", end="")
    seconds_buffered = (len(window) / sample_rate) if sample_rate > 0 else 0.0
    print(
        f"PDM Live Samples | port={port} baud={baud} | buffered={len(window)} "
        f"(~{seconds_buffered:.2f}s) total={total} dropped={dropped}"
    )

    if not window:
        print("Waiting for samples...")
        return

    values = list(window)
    vmin = min(values)
    vmax = max(values)
    mean = statistics.fmean(values)
    rms = math.sqrt(sum(v * v for v in values) / len(values))

    print(f"min={vmin:6d} max={vmax:6d} mean={mean:9.2f} rms={rms:9.2f}")
    print("Recent samples:")

    samples_per_line = 24
    for i in range(0, len(values), samples_per_line):
        chunk = values[i : i + samples_per_line]
        print(" ".join(f"{v:6d}" for v in chunk))


def read_available_samples(ser: serial.Serial, window: deque[int], burst_reads: int) -> tuple[int, int]:
    read_count = 0
    dropped_count = 0

    for _ in range(burst_reads):
        raw = ser.readline().decode("utf-8", errors="ignore")
        if not raw:
            break
        samples = parse_samples(raw)
        if samples:
            window.extend(samples)
            read_count += len(samples)
        else:
            dropped_count += 1

    return read_count, dropped_count


def run_plot_mode(
    ser: serial.Serial,
    port: str,
    baud: int,
    sample_rate: int,
    window: deque[int],
    refresh_ms: int,
    burst_reads: int,
) -> int:
    try:
        import matplotlib.pyplot as plt
    except ImportError as exc:
        raise SystemExit(
            "matplotlib is not installed. Run: pip install matplotlib"
        ) from exc

    plt.ion()
    fig, (ax_wave, ax_hist) = plt.subplots(2, 1, figsize=(12, 7), constrained_layout=True)
    fig.canvas.manager.set_window_title("PDM Live Plot")

    line_wave, = ax_wave.plot([], [], lw=1.0)
    ax_wave.set_title("PDM Waveform (rolling window)")
    ax_wave.set_xlabel("Sample index")
    ax_wave.set_ylabel("Amplitude")
    ax_wave.grid(True, alpha=0.25)

    ax_hist.set_title("Amplitude Histogram")
    ax_hist.set_xlabel("Amplitude")
    ax_hist.set_ylabel("Count")
    ax_hist.grid(True, alpha=0.25)

    info_text = fig.text(0.01, 0.99, "", va="top", ha="left", fontsize=10)

    total_samples = 0
    dropped_lines = 0

    while plt.fignum_exists(fig.number):
        read_count, dropped_count = read_available_samples(ser, window, burst_reads)
        total_samples += read_count
        dropped_lines += dropped_count

        if window:
            values = list(window)
            x = list(range(len(values)))
            line_wave.set_data(x, values)
            ax_wave.set_xlim(0, max(1, len(values) - 1))

            vmin = min(values)
            vmax = max(values)
            span = max(200, vmax - vmin)
            pad = max(100, int(span * 0.2))
            ax_wave.set_ylim(vmin - pad, vmax + pad)

            ax_hist.cla()
            ax_hist.hist(values, bins=40)
            ax_hist.set_title("Amplitude Histogram")
            ax_hist.set_xlabel("Amplitude")
            ax_hist.set_ylabel("Count")
            ax_hist.grid(True, alpha=0.25)

            mean = statistics.fmean(values)
            rms = math.sqrt(sum(v * v for v in values) / len(values))
            seconds_buffered = (len(values) / sample_rate) if sample_rate > 0 else 0.0
            info_text.set_text(
                f"port={port} baud={baud} buffered={len(values)} (~{seconds_buffered:.2f}s) "
                f"total={total_samples} dropped={dropped_lines}"
                f"\nmin={vmin} max={vmax} mean={mean:.2f} rms={rms:.2f}"
            )
        else:
            info_text.set_text(
                f"port={port} baud={baud} buffered=0 total={total_samples} dropped={dropped_lines}"
                "\nWaiting for samples..."
            )

        fig.canvas.draw_idle()
        plt.pause(refresh_ms / 1000.0)

    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Live serial monitor for microphone samples")
    parser.add_argument("--port", default=None, help="Serial port (e.g. COM3)")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate")
    parser.add_argument(
        "--sample-rate",
        type=int,
        default=8000,
        help="Expected microphone sample rate in Hz (used for duration-based window)",
    )
    parser.add_argument(
        "--seconds",
        type=float,
        default=1.0,
        help="Buffer duration to keep in seconds (minimum is 1.0)",
    )
    parser.add_argument(
        "--window",
        type=int,
        default=0,
        help="Optional explicit number of recent samples to keep (overrides --seconds)",
    )
    parser.add_argument("--refresh-ms", type=int, default=120, help="Screen refresh period")
    parser.add_argument(
        "--mode",
        choices=["text", "plot"],
        default="plot",
        help="Display mode: text console or live plot",
    )
    parser.add_argument(
        "--burst-reads",
        type=int,
        default=300,
        help="Max serial lines to consume per refresh cycle",
    )
    args = parser.parse_args()

    port = args.port or detect_default_port()
    if not port:
        raise SystemExit(
            "No serial port detected. Make sure your device is connected or use --port COM3 explicitly."
        )

    seconds = max(1.0, args.seconds)
    computed_window = int(args.sample_rate * seconds)
    window_size = args.window if args.window > 0 else computed_window
    if window_size <= 0:
        raise SystemExit("Invalid buffer size. Use --window > 0 or valid --sample-rate/--seconds.")

    window: deque[int] = deque(maxlen=window_size)
    total_samples = 0
    dropped_lines = 0

    with serial.Serial(port, args.baud, timeout=0.02) as ser:
        # Reset any stale boot messages in the buffer.
        ser.reset_input_buffer()

        if args.mode == "plot":
            return run_plot_mode(
                ser=ser,
                port=port,
                baud=args.baud,
                sample_rate=args.sample_rate,
                window=window,
                refresh_ms=args.refresh_ms,
                burst_reads=args.burst_reads,
            )

        next_refresh = time.monotonic()
        while True:
            read_count, dropped_count = read_available_samples(ser, window, args.burst_reads)
            total_samples += read_count
            dropped_lines += dropped_count

            now = time.monotonic()
            if now >= next_refresh:
                render_screen(
                    port,
                    args.baud,
                    args.sample_rate,
                    window,
                    total_samples,
                    dropped_lines,
                )
                next_refresh = now + (args.refresh_ms / 1000.0)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\nStopped.")
        raise SystemExit(0)