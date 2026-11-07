#!/usr/bin/env python3
"""Check stack headroom and audio counters over a long serial log.

Every 10 seconds the firmware prints a line like::

    I (10000) telemetry: stack free: audio_io=1820 filter=2400 alignment=3100 config=900 telemetry=1500

The numbers are the fewest bytes each task's stack has had free so far (its
high-water mark). This tool reads a saved ``idf.py monitor`` log and answers
the stack-headroom bench check (#265):

- the smallest free stack seen for each task, flagging any under 512 bytes
  (and any 0, which means the task never started),
- whether ``under=`` or ``drop=`` grew between the first and last telemetry line.

It ends with ``RESULT: PASS`` or ``RESULT: CHECK`` and the reasons. The exit
code is 0 for PASS and 1 for CHECK.

Usage:
    python tools/stack_check.py run.log
    python tools/stack_check.py run.log --min-free 1024
"""

import argparse
import sys

from telemetry_to_csv import parse_line

STACK_MARKER = "stack free:"
DEFAULT_MIN_FREE = 512


def parse_stack_line(line):
    """Return {task: bytes free} for a ``stack free:`` line, or None."""
    if STACK_MARKER not in line:
        return None
    fields = {}
    for pair in line.split(STACK_MARKER, 1)[1].split():
        task, sep, value = pair.partition("=")
        if sep and task and value.isdigit():
            fields[task] = int(value)
    return fields or None


def check(lines):
    """Return the smallest free stack per task and the under/drop growth."""
    smallest = {}
    stack_lines = 0
    first = {}
    last = {}
    for line in lines:
        stacks = parse_stack_line(line)
        if stacks is not None:
            stack_lines += 1
            for task, free in stacks.items():
                smallest[task] = min(free, smallest.get(task, free))
            continue
        fields = parse_line(line)
        if fields is None:
            continue
        for key in ("under", "drop"):
            value = fields.get(key)
            if isinstance(value, (int, float)):
                first.setdefault(key, value)
                last[key] = value
    growth = {key: last[key] - first[key] for key in last}
    return {"stack_lines": stack_lines, "smallest": smallest, "growth": growth}


def problems(result, min_free=DEFAULT_MIN_FREE):
    """Return the reasons the run needs a closer look (empty when it passed)."""
    reasons = []
    if result["stack_lines"] == 0:
        reasons.append("no 'stack free:' lines found")
    for task, free in result["smallest"].items():
        if free == 0:
            reasons.append(f"{task} never started (0 bytes)")
        elif free < min_free:
            reasons.append(f"{task} has only {free} bytes free")
    for key in ("under", "drop"):
        if key not in result["growth"]:
            reasons.append(f"no telemetry line with {key}=")
        elif result["growth"][key] > 0:
            reasons.append(f"{key} grew by {result['growth'][key]}")
    return reasons


def format_report(result, min_free=DEFAULT_MIN_FREE):
    """Return the report as printable lines."""
    out = [f"stack free lines: {result['stack_lines']}"]
    for task, free in sorted(result["smallest"].items(), key=lambda item: item[1]):
        flag = ""
        if free == 0:
            flag = "  <-- never started"
        elif free < min_free:
            flag = f"  <-- under {min_free} bytes"
        out.append(f"  {task}: smallest {free} bytes free{flag}")
    for key in ("under", "drop"):
        if key in result["growth"]:
            out.append(f"{key} grew by {result['growth'][key]}")
    reasons = problems(result, min_free)
    out.append("RESULT: PASS" if not reasons else "RESULT: CHECK (" + "; ".join(reasons) + ")")
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("path", help="saved idf.py monitor output")
    parser.add_argument("--min-free", type=int, default=DEFAULT_MIN_FREE,
                        help=f"flag tasks with fewer free bytes (default {DEFAULT_MIN_FREE})")
    args = parser.parse_args()
    if args.min_free < 1:
        sys.exit("--min-free must be 1 or more")
    try:
        with open(args.path, encoding="utf-8", errors="replace") as log:
            result = check(log)
    except OSError as error:
        sys.exit(f"Cannot read log: {error}")
    print("\n".join(format_report(result, args.min_free)))
    sys.exit(0 if not problems(result, args.min_free) else 1)


if __name__ == "__main__":
    main()
