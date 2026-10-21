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


def dbfs_levels(samples):
    """Return (peak_dbfs, rms_dbfs) of samples; silence gives -infinity."""
    peak = float(np.max(np.abs(samples))) if samples.size else 0.0
    rms = float(np.sqrt(np.mean(samples ** 2))) if samples.size else 0.0
    return (
        20.0 * math.log10(peak) if peak > 0 else -math.inf,
        20.0 * math.log10(rms) if rms > 0 else -math.inf,
    )


def summarize(path):
    """Return recording metadata and peak/RMS dBFS across all channels.

    ``channels_dbfs`` also lists the levels of each channel on its own.
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
    peak_dbfs, rms_dbfs = dbfs_levels(samples)
    # Samples are interleaved, so each column of (-1, channels) is a channel.
    per_channel = [dbfs_levels(column) for column in samples.reshape(-1, channels).T]
    return {
        "file": str(path),
        "channels": channels,
        "sample_rate": rate,
        "duration": frames / rate,
        "peak_dbfs": peak_dbfs,
        "rms_dbfs": rms_dbfs,
        "channels_dbfs": [
            {"peak_dbfs": peak, "rms_dbfs": rms} for peak, rms in per_channel
        ],
    }


def json_safe(value):
    """Replace -infinity with None, including inside lists and dicts."""
    if isinstance(value, float) and math.isinf(value):
        return None
    if isinstance(value, dict):
        return {key: json_safe(item) for key, item in value.items()}
    if isinstance(value, list):
        return [json_safe(item) for item in value]
    return value


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
        print(json.dumps(json_safe(info)))
        return
    print(f"file:        {info['file']}")
    print(f"channels:    {info['channels']}")
    print(f"sample rate: {info['sample_rate']} Hz")
    print(f"duration:    {info['duration']:.2f} s")
    print(f"peak:        {info['peak_dbfs']:.1f} dBFS")
    print(f"rms:         {info['rms_dbfs']:.1f} dBFS")
    if info["channels"] > 1:
        for number, levels in enumerate(info["channels_dbfs"], start=1):
            print(
                f"channel {number}:   peak {levels['peak_dbfs']:.1f} dBFS, "
                f"rms {levels['rms_dbfs']:.1f} dBFS"
            )


if __name__ == "__main__":
    main()
