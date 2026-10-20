#!/usr/bin/env python3
"""Summarize the format and levels of a 16-bit PCM WAV recording.

Usage:
    python tools/wav_info.py room.wav
"""

import argparse
import json
import math
import sys
import wave

import numpy as np


def summarize(path):
    """Return recording metadata and peak/RMS dBFS across all channels.

    Levels use 32768 as the 16-bit full-scale amplitude. Silence, including
    an empty recording, has -infinity dBFS. Unsupported sample widths raise
    ValueError rather than guessing a conversion.
    """
    with wave.open(str(path), "rb") as wav:
        if wav.getsampwidth() != 2:
            raise ValueError(f"{path}: expected 16-bit PCM")
        channels = wav.getnchannels()
        rate = wav.getframerate()
        frames = wav.getnframes()
        raw = wav.readframes(frames)

    # Cast before squaring or taking abs, avoiding 16-bit integer overflow.
    samples = np.frombuffer(raw, dtype="<i2").astype(np.float64) / 32768.0
    peak = float(np.max(np.abs(samples))) if samples.size else 0.0
    rms = float(np.sqrt(np.mean(samples ** 2))) if samples.size else 0.0
    return {
        "file": str(path),
        "channels": channels,
        "sample_rate": rate,
        "duration": frames / rate,
        "peak_dbfs": 20.0 * math.log10(peak) if peak > 0 else -math.inf,
        "rms_dbfs": 20.0 * math.log10(rms) if rms > 0 else -math.inf,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="16-bit PCM WAV recording to summarize")
    parser.add_argument(
        "--json",
        action="store_true",
        help="print metadata as JSON",
    )
    args = parser.parse_args()
    try:
        info = summarize(args.path)
    except (OSError, ValueError, wave.Error, EOFError) as error:
        sys.exit(f"Cannot summarize WAV: {error}")

    if args.json:
        json_info={
            key:None if isinstance(value,float)and math.isinf(value)else value
            for key,value in info.items()
        }
        print(json.dumps(json_info))
        return
    print(f"file:        {info['file']}")
    print(f"channels:    {info['channels']}")
    print(f"sample rate: {info['sample_rate']} Hz")
    print(f"duration:    {info['duration']:.2f} s")
    print(f"peak:        {info['peak_dbfs']:.1f} dBFS")
    print(f"rms:         {info['rms_dbfs']:.1f} dBFS")


if __name__ == "__main__":
    main()
