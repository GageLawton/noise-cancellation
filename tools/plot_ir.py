#!/usr/bin/env python3
"""Draw a measured impulse response as a picture.

Reads the ``index,value`` CSV that ``ir_from_log.py --csv`` writes and plots
each tap against its delay in milliseconds, with a dashed line at the peak.
The picture makes the speaker-to-mic delay, and how quickly the response
dies away, easy to see (bench check #110). With ``--compare`` a second
response is drawn on the same axes, to see what moved when the speaker-to-mic
path changed.

Needs matplotlib, which only bench users need, so it isn't in
requirements.txt:
    pip install matplotlib

Usage:
    python tools/ir_from_log.py monitor.log --csv ir.csv
    python tools/plot_ir.py ir.csv --out ir.png
    python tools/plot_ir.py ir.csv --rate 16000   # if the mic ran at 16 kHz
    python tools/plot_ir.py old.csv --compare new.csv --out moved.png
"""

import argparse
import csv
import os
import sys

SAMPLE_RATE_HZ = 8000


def load_ir_csv(path):
    """Return the CSV's rows as a list of (index, value) pairs.

    The first row is skipped if it is the ``index,value`` header.
    """
    pairs = []
    with open(path, newline="", encoding="utf-8") as f:
        for row in csv.reader(f):
            if not row or row[0].strip() == "index":
                continue
            pairs.append((int(row[0]), float(row[1])))
    return pairs


def delays_ms(pairs, rate):
    """Return each tap's delay in milliseconds at rate samples per second."""
    return [1000.0 * index / rate for index, _ in pairs]


def peak_pair(pairs):
    """Return the (index, value) pair with the largest absolute value."""
    return max(pairs, key=lambda pair: abs(pair[1]))


def title_for(pairs_a, pairs_b, rate):
    """Return the plot title for one response, or for two when pairs_b is set."""
    index_a, value_a = peak_pair(pairs_a)
    delay_a = 1000.0 * index_a / rate
    if pairs_b is None:
        return f"Impulse response: peak {value_a:.3f} at {delay_a:.3f} ms"
    delay_b = 1000.0 * peak_pair(pairs_b)[0] / rate
    return f"Impulse responses: peak moved by {delay_b - delay_a:.3f} ms"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="CSV of index,value rows from ir_from_log.py")
    parser.add_argument("--out", default="ir.png", help="PNG to write (default ir.png)")
    parser.add_argument(
        "--rate",
        type=int,
        default=SAMPLE_RATE_HZ,
        help=f"sample rate in Hz the impulse response was measured at "
        f"(default {SAMPLE_RATE_HZ})",
    )
    parser.add_argument(
        "--compare",
        metavar="OTHER.csv",
        help="a second impulse response CSV to draw on the same axes",
    )
    args = parser.parse_args()
    if args.rate <= 0:
        sys.exit("--rate must be a positive number of samples per second")

    try:
        import matplotlib

        matplotlib.use("Agg")  # write a file; no window needed
        import matplotlib.pyplot as plt
    except ImportError:
        sys.exit("plot_ir.py needs matplotlib: pip install matplotlib")

    paths = [args.path] + ([args.compare] if args.compare else [])
    responses = []
    for path in paths:
        try:
            pairs = load_ir_csv(path)
        except (OSError, ValueError, IndexError) as error:
            sys.exit(f"Cannot read impulse response CSV: {error}")
        if not pairs:
            sys.exit(f"No taps found in {path}")
        responses.append(pairs)

    fig, ax = plt.subplots(figsize=(8, 4))
    for path, pairs in zip(paths, responses):
        line = ax.plot(
            delays_ms(pairs, args.rate),
            [value for _, value in pairs],
            marker=".",
            label=os.path.basename(path),
        )[0]
        # One response keeps the gray peak line; two use each line's colour.
        color = line.get_color() if args.compare else "gray"
        ax.axvline(1000.0 * peak_pair(pairs)[0] / args.rate, linestyle="--", color=color)
    if args.compare:
        ax.legend()
    ax.set_title(title_for(responses[0], responses[1] if args.compare else None, args.rate))
    ax.set_xlabel(f"Delay (ms at {args.rate} Hz)")
    ax.set_ylabel("Tap value")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(args.out)
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
