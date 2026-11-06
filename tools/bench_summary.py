#!/usr/bin/env python3
"""Summarize a long telemetry log from a bench check.

Bench checks like #92 run for 10 minutes and print 600 telemetry lines.
This tool answers the usual questions directly: did anything drop, how
loud was the mic, and how often was the frequency tracker locked? For the
lock-threshold check (#264) it also splits the hum level by lock=1 and
lock=0, shows the tracked frequency while locked, and turns the hum level
into the amplitude that tracking.min_lock_amplitude is compared against.

Usage:
    python tools/bench_summary.py run.log
    python tools/bench_summary.py run.log --json
"""

import argparse
import json
import sys

from telemetry_to_csv import parse_line

COUNTERS = (
    "blocks",
    "under",
    "drop",
    "clip",
    "faults",
    "syncrx",
    "missed",
    "bad",
    "sync_heard",
    "sync_sent",
)


def read_telemetry(lines):
    """Return the parsed fields of every telemetry line, in order."""
    return [fields for fields in map(parse_line, lines) if fields is not None]


def level_range(records, key):
    """Return (min, mean, max) of the numeric values of key, or None if none."""
    values = [r[key] for r in records if isinstance(r.get(key), (int, float))]
    if not values:
        return None
    return (min(values), sum(values) / len(values), max(values))


def dbfs_to_amplitude(dbfs):
    """Return the linear amplitude (full scale = 1) of a level in dBFS."""
    return 10 ** (dbfs / 20)


def summarize(records):
    """Return a dict of summary numbers for a non-empty list of records.

    counters maps each of COUNTERS that appears to (first, last, growth).
    mic, resid, hum and offset are each (min, mean, max) or None, as are
    freq_locked, hum_locked and hum_unlocked (the same over only the lines
    with lock=1 or lock=0). lock_fraction is a number from 0 to 1
    or None, and states lists each state in order of first appearance.
    """
    summary = {"lines": len(records), "seconds": len(records)}  # one line per second

    counters = {}
    for key in COUNTERS:
        values = [r[key] for r in records if isinstance(r.get(key), (int, float))]
        if values:
            counters[key] = (values[0], values[-1], values[-1] - values[0])
    summary["counters"] = counters

    summary["mic"] = level_range(records, "mic")
    summary["resid"] = level_range(records, "resid")
    summary["hum"] = level_range(records, "hum")
    summary["offset"] = level_range(records, "offset")

    locked = [r for r in records if r.get("lock") == 1]
    unlocked = [r for r in records if r.get("lock") == 0]
    summary["freq_locked"] = level_range(locked, "freq")
    summary["hum_locked"] = level_range(locked, "hum")
    summary["hum_unlocked"] = level_range(unlocked, "hum")

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
    for key in ("resid", "hum"):
        if summary[key] is not None:
            low, mean, high = summary[key]
            lines.append(
                f"{key} dBFS: min {low:.1f}, mean {mean:.1f}, max {high:.1f}"
            )
    if summary["offset"] is not None:
        low, mean, high = summary["offset"]
        lines.append(f"clock offset us: min {low:.0f}, mean {mean:.0f}, max {high:.0f}")
    if summary["lock_fraction"] is not None:
        lines.append(f"locked: {100 * summary['lock_fraction']:.0f}% of lines")
    if summary["freq_locked"] is not None:
        low, mean, high = summary["freq_locked"]
        lines.append(f"freq Hz while locked: min {low:.2f}, mean {mean:.2f}, max {high:.2f}")
    for key, label in (("hum_locked", "locked"), ("hum_unlocked", "unlocked")):
        if summary[key] is not None:
            low, mean, high = summary[key]
            lines.append(
                f"hum dBFS while {label}: min {low:.1f}, mean {mean:.1f}, max {high:.1f}"
            )
    if summary["hum"] is not None:
        mean = summary["hum"][1]
        lines.append(
            f"hum amplitude: mean {dbfs_to_amplitude(mean):.3f} "
            "(tracking.min_lock_amplitude must be below this to lock)"
        )
    lines.append("states: " + " -> ".join(str(s) for s in summary["states"]))
    return lines


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="serial log containing telemetry: lines")
    parser.add_argument("--json", action="store_true",
                        help="print the summary as one JSON object")
    args = parser.parse_args()

    try:
        with open(args.path, encoding="utf-8", errors="replace") as log:
            records = read_telemetry(log)
    except OSError as error:
        sys.exit(f"Cannot read log: {error}")

    if not records:
        sys.exit("No telemetry: lines found")

    summary = summarize(records)
    if args.json:
        print(json.dumps(summary))
    else:
        print("\n".join(format_summary(summary)))


if __name__ == "__main__":
    main()
