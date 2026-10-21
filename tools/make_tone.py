#!/usr/bin/env python3
"""Generate a 16-bit PCM WAV file containing a single test tone or a sweep.

Testing the offline analysis (analyze.py) needs recordings with a known
tone in them. On a laptop, with no hardware, this is the quickest way to
get one.

Usage:
    ./make_tone.py tone.wav --freq 120
    ./make_tone.py tone_stereo.wav --freq 60 --stereo
    ./make_tone.py sweep.wav --sweep 20 500 --seconds 10
"""

import argparse
import wave

import numpy as np


def make_tone(freq_hz, seconds, rate, amplitude, stereo=False, noise=0.0):
    """Sine wave as int16 samples, interleaved if stereo.

    A stereo file carries the tone on the left channel and silence on the
    right: that asymmetry makes it obvious when a reader mixes channels
    together or treats the interleaved stream as mono.
    """
    t = np.arange(round(seconds * rate), dtype=np.float64) / rate
    tone = amplitude * np.sin(2.0 * np.pi * freq_hz * t)
    if noise > 0.0:
        rng = np.random.default_rng(0)
        tone = tone + noise * rng.standard_normal(t.size)
        tone = np.clip(tone, -1.0, 1.0)
    # 32767 rather than 32768: the positive peak of an int16 is one short
    # of the negative one, so scaling by 32768 can wrap a full-scale tone.
    samples = np.round(tone * 32767.0).astype(np.int16)

    if not stereo:
        return samples

    interleaved = np.zeros(samples.size * 2, dtype=np.int16)
    interleaved[0::2] = samples
    return interleaved


def make_sweep(start_hz, end_hz, seconds, rate, amplitude):
    """Linear sweep from start_hz to end_hz as int16 samples.

    Played through the speaker and recorded, it shows which frequencies the
    speaker can actually reproduce. The phase is the integral of the
    frequency, which rises linearly from start_hz to end_hz over seconds.
    """
    t = np.arange(round(seconds * rate), dtype=np.float64) / rate
    phase = 2.0 * np.pi * (start_hz * t + (end_hz - start_hz) * t**2 / (2.0 * seconds))
    return np.round(amplitude * np.sin(phase) * 32767.0).astype(np.int16)


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("output", help="path of the WAV file to write")
    p.add_argument("--freq", type=float, default=60.0,
                   help="tone frequency in Hz (default: 60)")
    p.add_argument("--seconds", type=float, default=3.0,
                   help="length in seconds (default: 3)")
    p.add_argument("--rate", type=int, default=8000,
                   help="sample rate in Hz (default: 8000)")
    p.add_argument("--amplitude", type=float, default=0.5,
                   help="amplitude from 0 to 1 (default: 0.5)")
    p.add_argument("--noise", type=float, default=0.0,
                   help="white noise level, 0 to 1 (default: 0, none)")
    p.add_argument("--stereo", action="store_true",
                   help="write the tone on the left channel and silence on the right")
    p.add_argument("--sweep", type=float, nargs=2, metavar=("START", "END"),
                   help="write a linear sweep from START to END Hz instead "
                        "(--freq is ignored)")
    args = p.parse_args()

    if args.freq <= 0:
        p.error("--freq must be greater than 0")
    if args.seconds <= 0:
        p.error("--seconds must be greater than 0")
    if args.rate <= 0:
        p.error("--rate must be greater than 0")
    if not 0.0 < args.amplitude <= 1.0:
        p.error("--amplitude must be greater than 0 and at most 1")
    if not 0.0 <= args.noise <= 1.0:
        p.error("--noise must be between 0 and 1")
    nyquist = args.rate / 2.0
    if args.sweep:
        if not all(0.0 < f < nyquist for f in args.sweep):
            p.error(f"--sweep frequencies must be above 0 and below {nyquist:.1f} Hz")
        if args.noise > 0.0:
            p.error("--noise is not supported with --sweep")
    elif args.freq >= nyquist:
        p.error(f"--freq must be below the Nyquist limit ({nyquist:.1f} Hz)")

    channels = 2 if args.stereo else 1
    if args.sweep:
        samples = make_sweep(args.sweep[0], args.sweep[1], args.seconds, args.rate,
                             args.amplitude)
        if args.stereo:
            interleaved = np.zeros(samples.size * 2, dtype=np.int16)
            interleaved[0::2] = samples
            samples = interleaved
    else:
        samples = make_tone(args.freq, args.seconds, args.rate, args.amplitude,
                            stereo=args.stereo, noise=args.noise)

    with wave.open(args.output, "wb") as w:
        w.setnchannels(channels)
        w.setsampwidth(2)
        w.setframerate(args.rate)
        w.writeframes(samples.tobytes())

    if args.sweep:
        what = f"sweep {args.sweep[0]:.2f}-{args.sweep[1]:.2f} Hz"
    else:
        what = f"{args.freq:.2f} Hz"
    print(f"{args.output}: {what}, {args.seconds:g} s, "
          f"{args.rate} Hz, {'stereo' if args.stereo else 'mono'}")


if __name__ == "__main__":
    main()
