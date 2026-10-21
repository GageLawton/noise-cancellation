#!/usr/bin/env python3
"""Simulate the one-tone FxLMS canceller to try step sizes without the board.

Bench check #301 picks the step size on the board. This runs the same
normalized FxLMS as components/dsp_core/include/anc/fxlms_canceller.hpp
against a 60 Hz hum, with a speaker-to-mic path that is just a delay, so
the trade-off shows up first on a laptop: a bigger step settles faster,
until the loop delay makes it blow up.

For each step size it prints how far the hum fell over the last second,
compared to the first 100 ms, or ``diverged`` if the mic signal ran away.

Usage:
    ./fxlms_sim.py --mu 0.001 0.005 0.02
    ./fxlms_sim.py --mu 0.005 --delay 100 --seconds 10
"""

import argparse

import numpy as np

POWER_LAMBDA = 0.01  # running power average, as in FxlmsCanceller
EPSILON = 1e-6  # kNlmsEpsilon
DIVERGED_ABOVE = 10.0  # any mic sample this large means the loop ran away


def simulate(mu, delay_samples=180, seconds=5.0, rate=8000, hum_hz=60.0, hum_amp=0.3):
    """Return the mic signal of a one-tone FxLMS run as a numpy array.

    The reference is a sine and a cosine at hum_hz with one weight each.
    The speaker reaches the mic delay_samples later, and the filtered
    reference is the reference delayed the same way (a perfect path
    model). Each sample the weights move by
    ``mu / (eps + power) * mic * filtered``, with the minus sign that makes
    the speaker oppose the hum.
    """
    n = round(seconds * rate)
    phase = 2.0 * np.pi * hum_hz * np.arange(n) / rate
    ref_sin = np.sin(phase)
    ref_cos = np.cos(phase)
    hum = hum_amp * np.sin(phase + 1.1)

    w_sin = 0.0
    w_cos = 0.0
    power = 0.0
    speaker = np.zeros(n)
    mic = np.zeros(n)
    for i in range(n):
        speaker[i] = w_sin * ref_sin[i] + w_cos * ref_cos[i]
        j = i - delay_samples
        heard = speaker[j] if j >= 0 else 0.0
        filtered_sin = ref_sin[j] if j >= 0 else 0.0
        filtered_cos = ref_cos[j] if j >= 0 else 0.0
        mic[i] = hum[i] + heard
        if not np.isfinite(mic[i]) or abs(mic[i]) > 1e6:
            mic[i:] = np.inf  # far past diverged; stop before it overflows
            break

        squares = filtered_sin * filtered_sin + filtered_cos * filtered_cos
        power += POWER_LAMBDA * (squares - power)
        # Like the firmware, the instantaneous value is a floor while the
        # average (which starts at 0) catches up.
        step = mu / (EPSILON + max(power, squares)) * mic[i]
        w_sin -= step * filtered_sin
        w_cos -= step * filtered_cos
    return mic


def diverged(mic):
    """True if any mic sample's magnitude passes DIVERGED_ABOVE."""
    return bool(np.any(~np.isfinite(mic)) or np.any(np.abs(mic) > DIVERGED_ABOVE))


def reduction_db(mic, rate=8000):
    """Hum level over the last second relative to the first 100 ms, in dB.

    Positive means the hum got quieter.
    """
    first = mic[: rate // 10]
    last = mic[-rate:]
    rms_first = np.sqrt(np.mean(first * first))
    rms_last = np.sqrt(np.mean(last * last))
    return 20.0 * np.log10(rms_first / max(rms_last, 1e-12))


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--mu", type=float, nargs="+", required=True,
                   help="one or more normalized step sizes to try")
    p.add_argument("--delay", type=int, default=180,
                   help="speaker-to-mic delay in samples (default: 180)")
    p.add_argument("--seconds", type=float, default=5.0,
                   help="length of each run in seconds (default: 5)")
    p.add_argument("--rate", type=int, default=8000,
                   help="sample rate in Hz (default: 8000)")
    p.add_argument("--hum", type=float, default=60.0,
                   help="hum frequency in Hz (default: 60)")
    args = p.parse_args()

    if args.delay < 0:
        p.error("--delay must be 0 or more")
    if args.rate <= 0:
        p.error("--rate must be greater than 0")
    if args.seconds < 1.0:
        p.error("--seconds must be at least 1 (the last second is measured)")

    for mu in args.mu:
        mic = simulate(mu, delay_samples=args.delay, seconds=args.seconds,
                       rate=args.rate, hum_hz=args.hum)
        if diverged(mic):
            print(f"mu {mu:g}: diverged")
        else:
            print(f"mu {mu:g}: {reduction_db(mic, args.rate):.1f} dB quieter")


if __name__ == "__main__":
    main()
