#!/usr/bin/env python3
"""Offline analysis for bench validation (design doc section 9.2).

Measures narrowband cancellation depth from two recordings made with the
external measurement mic: one with the canceller muted (baseline) and one
with it running and converged.

The system's own mic cannot be used for this -- the control loop actively
drives it toward zero, so measuring with it is circular.

Usage:
    ./analyze.py baseline.wav cancelled.wav --freq 60
"""

import argparse
import sys
import wave

import numpy as np


def read_wav(path):
    with wave.open(path, "rb") as w:
        if w.getsampwidth() != 2:
            sys.exit(f"{path}: expected 16-bit PCM")
        rate = w.getframerate()
        frames = w.readframes(w.getnframes())
    data = np.frombuffer(frames, dtype=np.int16).astype(np.float64) / 32768.0
    return data, rate


def narrowband_rms(signal, rate, freq_hz, bandwidth_hz=2.0):
    """RMS within a narrow band around freq_hz.

    Narrowband rather than broadband RMS: unrelated room noise would
    otherwise dilute the measurement and understate real cancellation.
    """
    spectrum = np.fft.rfft(signal * np.hanning(len(signal)))
    freqs = np.fft.rfftfreq(len(signal), 1.0 / rate)

    band = (freqs >= freq_hz - bandwidth_hz) & (freqs <= freq_hz + bandwidth_hz)
    if not band.any():
        sys.exit(f"no FFT bins near {freq_hz} Hz -- recording too short?")

    # Parseval: band energy back to a time-domain-comparable RMS
    return np.sqrt(np.sum(np.abs(spectrum[band]) ** 2)) / len(signal)


def dominant_peak(signal, rate, lo=20.0, hi=1000.0):
    """Strongest tone in a range -- use this to find what to cancel."""
    spectrum = np.abs(np.fft.rfft(signal * np.hanning(len(signal))))
    freqs = np.fft.rfftfreq(len(signal), 1.0 / rate)
    band = (freqs >= lo) & (freqs <= hi)
    return freqs[band][np.argmax(spectrum[band])]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("baseline", help="recording with canceller muted")
    p.add_argument("cancelled", nargs="?", help="recording with canceller running")
    p.add_argument("--freq", type=float,
                   help="target frequency in Hz (default: auto-detect)")
    args = p.parse_args()

    base, rate = read_wav(args.baseline)

    freq = args.freq if args.freq else dominant_peak(base, rate)
    print(f"target frequency: {freq:.2f} Hz")

    if not args.cancelled:
        # Survey mode: just report what is in the room. This is step one
        # of the project -- measure before picking a tone to cancel.
        print("\ntop peaks in baseline:")
        spectrum = np.abs(np.fft.rfft(base * np.hanning(len(base))))
        freqs = np.fft.rfftfreq(len(base), 1.0 / rate)
        band = (freqs >= 20) & (freqs <= 1000)
        idx = np.argsort(spectrum[band])[-8:][::-1]
        for i in idx:
            print(f"  {freqs[band][i]:7.2f} Hz  {spectrum[band][i]:.4f}")
        return

    canc, rate2 = read_wav(args.cancelled)
    if rate2 != rate:
        sys.exit("sample rate mismatch between recordings")

    rms_off = narrowband_rms(base, rate, freq)
    rms_on = narrowband_rms(canc, rate, freq)

    if rms_on <= 0:
        sys.exit("cancelled recording has no energy at target frequency")

    depth_db = 20.0 * np.log10(rms_off / rms_on)
    print(f"baseline RMS:  {rms_off:.6f}")
    print(f"cancelled RMS: {rms_on:.6f}")
    print(f"cancellation depth: {depth_db:.2f} dB")


if __name__ == "__main__":
    main()
