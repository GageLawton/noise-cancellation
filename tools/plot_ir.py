#!/usr/bin/env python3
"""Draw a measured impulse response as a picture.

Reads the ``index,value`` CSV that ``ir_from_log.py --csv`` writes and plots
each tap against its delay in milliseconds, with a dashed line at the peak.
The picture makes the speaker-to-mic delay, and how quickly the response
dies away, easy to see (bench check #110).

Needs matplotlib, which only bench users need, so it isn't in
requirements.txt:
    pip install matplotlib

Usage:
    python tools/ir_from_log.py monitor.log --csv ir.csv
    python tools/plot_ir.py ir.csv --out ir.png
"""

import argparse
import csv
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="CSV of index,value rows from ir_from_log.py")
    parser.add_argument("--out", default="ir.png", help="PNG to write (default ir.png)")
    args = parser.parse_args()

    try:
        import matplotlib

        matplotlib.use("Agg")  # write a file; no window needed
        import matplotlib.pyplot as plt
    except ImportError:
        sys.exit("plot_ir.py needs matplotlib: pip install matplotlib")

    try:
        pairs = load_ir_csv(args.path)
    except (OSError, ValueError, IndexError) as error:
        sys.exit(f"Cannot read impulse response CSV: {error}")
    if not pairs:
        sys.exit("No taps found in the CSV")

    delays_ms = [1000.0 * index / SAMPLE_RATE_HZ for index, _ in pairs]
    values = [value for _, value in pairs]
    peak = max(range(len(values)), key=lambda i: abs(values[i]))

    fig, ax = plt.subplots(figsize=(8, 4))
    ax.plot(delays_ms, values, marker=".")
    ax.axvline(delays_ms[peak], linestyle="--", color="gray")
    ax.set_title(
        f"Impulse response: peak {values[peak]:.3f} at {delays_ms[peak]:.3f} ms"
    )
    ax.set_xlabel(f"Delay (ms at {SAMPLE_RATE_HZ} Hz)")
    ax.set_ylabel("Tap value")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(args.out)
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
