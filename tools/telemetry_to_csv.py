#!/usr/bin/env python3
"""Turn the firmware's telemetry lines into a CSV table.

The firmware prints a telemetry line every second, for example::

    I (12345) telemetry: state=RUN conv=0 resid=-42.1dBFS blocks=2500 clip=0

This tool keeps every ``key=value`` pair after ``telemetry:``, so new fields
show up as new columns without changing the tool. Units are stripped from
numbers (``-42.1dBFS`` becomes ``-42.1``) so the CSV plots directly. Other
log lines are skipped.

Usage:
    python tools/telemetry_to_csv.py monitor.log
    python tools/telemetry_to_csv.py monitor.log --out telemetry.csv
"""

import argparse
import csv
import re
import sys

MARKER = "telemetry:"

# A number at the start of a value, with an optional unit after it.
NUMBER_WITH_UNIT = re.compile(r"([-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?)[A-Za-z%/]*")


def parse_value(text):
    """Return text as an int or float if it is a number (unit stripped).

    Anything else, like ``RUN``, comes back unchanged as text.
    """
    match = NUMBER_WITH_UNIT.fullmatch(text)
    if not match:
        return text
    number = match.group(1)
    try:
        return int(number)
    except ValueError:
        return float(number)


def parse_line(line):
    """Return {key: value} for a telemetry line, or None for any other line."""
    if MARKER not in line:
        return None
    fields = {}
    for pair in line.split(MARKER, 1)[1].split():
        key, sep, value = pair.partition("=")
        if sep and key:
            fields[key] = parse_value(value)
    return fields


def to_rows(lines):
    """Return (columns, rows) for the telemetry lines among lines.

    columns starts with ``line`` (the 1-based line number in the log), then
    every key in order of first appearance. A row leaves a key blank if its
    line didn't have it.
    """
    columns = ["line"]
    rows = []
    for number, line in enumerate(lines, start=1):
        fields = parse_line(line)
        if fields is None:
            continue
        for key in fields:
            if key not in columns:
                columns.append(key)
        rows.append({"line": number, **fields})
    return columns, rows


def write_csv(columns, rows, out):
    writer = csv.DictWriter(out, fieldnames=columns, restval="")
    writer.writeheader()
    writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="serial log containing telemetry: lines")
    parser.add_argument("--out", metavar="CSV", help="write here instead of stdout")
    args = parser.parse_args()

    try:
        with open(args.path, encoding="utf-8", errors="replace") as log:
            columns, rows = to_rows(log)
    except OSError as error:
        sys.exit(f"Cannot read log: {error}")

    if not rows:
        sys.exit("No telemetry: lines found")

    if args.out:
        with open(args.out, "w", newline="", encoding="utf-8") as out:
            write_csv(columns, rows, out)
    else:
        write_csv(columns, rows, sys.stdout)


if __name__ == "__main__":
    main()
