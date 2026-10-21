#!/usr/bin/env python3
"""Plot what frequencies a recording holds.

``analyze.py`` lists the top peaks as text. A picture of the whole spectrum
makes hums and their harmonics easy to spot, for example during the room
survey (bench check #300). Light dashed lines mark 60, 120 and 180 Hz, where
mains hum and its harmonics usually sit.

Needs matplotlib, which only bench users need, so it isn't in
requirements.txt:
    pip install matplotlib

Usage:
    python tools/spectrum.py recording.wav
    python tools/spectrum.py recording.wav --out room.png --max-hz 500
"""

import argparse
import os
import sys

import numpy as np
from analyze import read_wav

HUM_LINES_HZ = (60, 120, 180)


def spectrum_db(signal, rate):
    """Return (freqs, levels_db) of signal from a Hann-windowed FFT.

    Levels are in dB relative to full scale: a full-scale sine reads about
    0 dB at its frequency. Each magnitude is floored at 1e-12 before the log
    so silence gives a finite level instead of -inf.
    """
    window = np.hanning(len(signal))
    spectrum = np.abs(np.fft.rfft(signal * window))
    # A sine of amplitude A peaks at A * sum(window) / 2 after windowing.
    magnitude = 2.0 * spectrum / np.sum(window)
    freqs = np.fft.rfftfreq(len(signal), 1.0 / rate)
    return freqs, 20.0 * np.log10(np.maximum(magnitude, 1e-12))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="16-bit PCM WAV recording")
    parser.add_argument(
        "--out", default="spectrum.png", help="PNG to write (default spectrum.png)"
    )
    parser.add_argument(
        "--max-hz",
        type=float,
        default=1000.0,
        help="highest frequency to plot in Hz (default 1000)",
    )
    args = parser.parse_args()
    if args.max_hz <= 0:
        sys.exit("--max-hz must be greater than 0")

    try:
        import matplotlib

        matplotlib.use("Agg")  # write a file; no window needed
        import matplotlib.pyplot as plt
    except ImportError:
        sys.exit("spectrum.py needs matplotlib: pip install matplotlib")

    signal, rate = read_wav(args.path)
    if len(signal) == 0:
        sys.exit("The recording has no samples")
    freqs, levels = spectrum_db(signal, rate)
    shown = freqs <= args.max_hz

    fig, ax = plt.subplots(figsize=(8, 4))
    ax.plot(freqs[shown], levels[shown])
    for hum_hz in HUM_LINES_HZ:
        if hum_hz <= args.max_hz:
            ax.axvline(hum_hz, linestyle="--", color="gray", alpha=0.4)
    ax.set_title(f"Spectrum of {os.path.basename(args.path)}")
    ax.set_xlabel("Frequency (Hz)")
    ax.set_ylabel("Level (dBFS)")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(args.out)
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
