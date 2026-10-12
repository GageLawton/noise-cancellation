#!/usr/bin/env python3
"""Pull the measured impulse response out of a serial log.

Calibration prints each tap as a line like ``ir,12,0.0431``, mixed in with
other log output. This tool collects those taps in index order and reports
the biggest one (the acoustic delay).

Usage:
    python tools/ir_from_log.py monitor.log
    python tools/ir_from_log.py monitor.log --csv ir.csv
"""

import argparse
import csv
import re
import sys

SAMPLE_RATE_HZ = 8000

# Matches anywhere in the line, so ESP-IDF prefixes like "I (1234) cal: "
# are skipped. The value accepts signs, decimals and exponents.
IR_LINE = re.compile(r"ir,(\d+),([-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?)")


def parse_ir(lines):
    """Return the impulse response taps found in lines, ordered by index.

    Lines that don't contain an ``ir,<int>,<float>`` record are ignored.
    """
    taps = {}
    for line in lines:
        match = IR_LINE.search(line)
        if match:
            taps[int(match.group(1))] = float(match.group(2))
    return [taps[index] for index in sorted(taps)]


def find_peak(taps):
    """Return (index, value) of the largest absolute tap, keeping its sign."""
    index = max(range(len(taps)), key=lambda i: abs(taps[i]))
    return index, taps[index]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="serial log containing ir,<index>,<value> lines")
    parser.add_argument("--csv", metavar="OUT", help="also write index,value rows")
    args = parser.parse_args()

    try:
        with open(args.path, encoding="utf-8", errors="replace") as log:
            taps = parse_ir(log)
    except OSError as error:
        sys.exit(f"Cannot read log: {error}")

    if not taps:
        sys.exit("No ir,<index>,<value> lines found")

    index, value = find_peak(taps)
    print(f"taps:  {len(taps)}")
    print(f"peak:  index {index}, value {value:.4f}")
    print(f"delay: {1000.0 * index / SAMPLE_RATE_HZ:.3f} ms at {SAMPLE_RATE_HZ} Hz")

    if args.csv:
        with open(args.csv, "w", newline="", encoding="utf-8") as out:
            writer = csv.writer(out)
            writer.writerow(["index", "value"])
            writer.writerows(enumerate(taps))


if __name__ == "__main__":
    main()
