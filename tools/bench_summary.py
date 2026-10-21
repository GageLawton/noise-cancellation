#!/usr/bin/env python3
"""Summarize a long telemetry log from a bench check.

Bench checks like #92 run for 10 minutes and print 600 telemetry lines.
This tool answers the usual questions directly: did anything drop, how
loud was the mic, and how often was the frequency tracker locked?

Usage:
    python tools/bench_summary.py run.log
"""

import argparse
import sys

from telemetry_to_csv import parse_line

COUNTERS = ("blocks", "under", "drop", "clip")


def read_telemetry(lines):
    """Return the parsed fields of every telemetry line, in order."""
    return [fields for fields in map(parse_line, lines) if fields is not None]


def summarize(records):
    """Return a dict of summary numbers for a non-empty list of records.

    counters maps each of COUNTERS that appears to (first, last, growth).
    mic is (min, mean, max) or None, lock_fraction is a number from 0 to 1
    or None, and states lists each state in order of first appearance.
    """
    summary = {"lines": len(records), "seconds": len(records)}  # one line per second

    counters = {}
    for key in COUNTERS:
        values = [r[key] for r in records if isinstance(r.get(key), (int, float))]
        if values:
            counters[key] = (values[0], values[-1], values[-1] - values[0])
    summary["counters"] = counters

    mic = [r["mic"] for r in records if isinstance(r.get("mic"), (int, float))]
    summary["mic"] = (min(mic), sum(mic) / len(mic), max(mic)) if mic else None

    locks = [r["lock"] for r in records if "lock" in r]
    summary["lock_fraction"] = (
        sum(1 for v in locks if v == 1) / len(locks) if locks else None
    )

    states = []
    for r in records:
        state = r.get("state")
        if state is not None and state not in states:
            states.append(state)
    summary["states"] = states
    return summary


def format_summary(summary):
    """Return the summary as printable lines."""
    lines = [
        f"telemetry lines: {summary['lines']} (about {summary['seconds']} s)",
    ]
    for key, (first, last, growth) in summary["counters"].items():
        lines.append(f"{key}: {first} -> {last} (+{growth})")
    if summary["mic"] is not None:
        low, mean, high = summary["mic"]
        lines.append(f"mic dBFS: min {low:.1f}, mean {mean:.1f}, max {high:.1f}")
    if summary["lock_fraction"] is not None:
        lines.append(f"locked: {100 * summary['lock_fraction']:.0f}% of lines")
    lines.append("states: " + " -> ".join(str(s) for s in summary["states"]))
    return lines


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="serial log containing telemetry: lines")
    args = parser.parse_args()

    try:
        with open(args.path, encoding="utf-8", errors="replace") as log:
            records = read_telemetry(log)
    except OSError as error:
        sys.exit(f"Cannot read log: {error}")

    if not records:
        sys.exit("No telemetry: lines found")

    print("\n".join(format_summary(summarize(records))))


if __name__ == "__main__":
    main()
