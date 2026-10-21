#!/usr/bin/env python3
"""Pull the measured impulse response out of a serial log.

Calibration prints each tap as a line like ``ir,12,0.0431``, mixed in with
other log output. This tool collects those taps in index order and reports
the biggest one (the acoustic delay) and how many taps follow it.

Usage:
    python tools/ir_from_log.py monitor.log
    python tools/ir_from_log.py monitor.log --csv ir.csv
    python tools/ir_from_log.py monitor.log --rate 16000
"""

import argparse
import csv
import re
import sys

SAMPLE_RATE_HZ = 8000

# The firmware warns when fewer taps than this follow the peak (kMinTailTaps
# in node_runtime.cpp): the speaker's ring after the peak wouldn't fit.
MIN_TAIL_TAPS = 64

# Matches anywhere in the line, so ESP-IDF prefixes like "I (1234) cal: "
# are skipped. The \b stops words that merely end in "ir", like "fir,3,0.5",
# from matching. The value accepts signs, decimals and exponents.
IR_LINE = re.compile(r"\bir,(\d+),([-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?)")


def parse_ir_taps(lines):
    """Return {index: value} for every ``ir,<int>,<float>`` record in lines.

    Other lines are ignored. A repeated index keeps the last value.
    """
    taps = {}
    for line in lines:
        match = IR_LINE.search(line)
        if match:
            taps[int(match.group(1))] = float(match.group(2))
    return taps


def parse_ir(lines):
    """Return the impulse response taps found in lines, ordered by index.

    Each value sits at its own index, so a dropped serial line leaves a 0.0
    gap instead of shifting every later tap (and the peak) down by one.
    """
    return taps_to_list(parse_ir_taps(lines))


def taps_to_list(taps):
    """Turn {index: value} into a list, filling missing indices with 0.0."""
    return [taps.get(index, 0.0) for index in range(max(taps, default=-1) + 1)]


def missing_indices(taps):
    """Return the indices below the highest one that have no record."""
    return [index for index in range(max(taps, default=-1) + 1) if index not in taps]


def find_peak(taps):
    """Return (index, value) of the largest absolute tap, keeping its sign."""
    index = max(range(len(taps)), key=lambda i: abs(taps[i]))
    return index, taps[index]


def taps_after_peak(taps):
    """Return how many taps follow the peak."""
    index, _ = find_peak(taps)
    return len(taps) - index - 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="serial log containing ir,<index>,<value> lines")
    parser.add_argument("--csv", metavar="OUT", help="also write index,value rows")
    parser.add_argument(
        "--rate",
        type=int,
        default=SAMPLE_RATE_HZ,
        help=f"sample rate in Hz, for the delay (default: {SAMPLE_RATE_HZ})",
    )
    args = parser.parse_args()

    try:
        with open(args.path, encoding="utf-8", errors="replace") as log:
            found = parse_ir_taps(log)
    except OSError as error:
        sys.exit(f"Cannot read log: {error}")

    if not found:
        sys.exit("No ir,<index>,<value> lines found")

    missing = missing_indices(found)
    if missing:
        print(
            f"warning: {len(missing)} taps missing (set to 0): {missing}",
            file=sys.stderr,
        )
    taps = taps_to_list(found)

    index, value = find_peak(taps)
    print(f"taps:  {len(taps)}")
    print(f"peak:  index {index}, value {value:.4f}")
    print(f"delay: {1000.0 * index / args.rate:.3f} ms at {args.rate} Hz")
    after = taps_after_peak(taps)
    print(f"after peak: {after} taps ({1000.0 * after / args.rate:.3f} ms)")
    if after < MIN_TAIL_TAPS:
        print(
            f"warning: fewer than {MIN_TAIL_TAPS} taps after the peak; "
            "try a larger dsp.secondary_path_taps",
            file=sys.stderr,
        )

    if args.csv:
        with open(args.csv, "w", newline="", encoding="utf-8") as out:
            writer = csv.writer(out)
            writer.writerow(["index", "value"])
            writer.writerows(enumerate(taps))


if __name__ == "__main__":
    main()
