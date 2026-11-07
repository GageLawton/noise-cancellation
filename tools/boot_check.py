#!/usr/bin/env python3
"""Check a first-boot serial log and answer the bench-check questions.

Save the output of ``idf.py monitor`` to a file (or copy it from the
terminal), then run this tool on it. It answers the four questions from
the first-boot bench check (#253) so you don't have to read the log by hand:

1. Which states did the node reach? (the ``runtime: A -> B`` lines)
2. What did calibration print, and were there any warnings?
3. Did telemetry show ``under=0`` and ``drop=0``?
4. Did anything crash or reboot?

It ends with ``RESULT: PASS`` when the node reached RUN with no crash,
underruns, drops or warnings, and ``RESULT: CHECK`` with the reasons
otherwise. The exit code is 0 for PASS and 1 for CHECK.

Usage:
    python tools/boot_check.py boot.log
"""

import argparse
import re
import sys

from telemetry_to_csv import parse_line

# "I (1234) runtime: BOOT -> INIT_HAL"
TRANSITION = re.compile(r"\bruntime: ([A-Z_]+) -> ([A-Z_]+)\s*$")
# "I (1234) runtime: calibration: peak at tap 171, value 0.4200, delay 10.69 ms"
CAL_PEAK = re.compile(
    r"calibration: peak at tap (\d+), value ([-+0-9.eE]+), delay ([-+0-9.eE]+) ms"
)
# ESP-IDF warning and error lines start with "W (" or "E (".
WARNING = re.compile(r"^\s*([WE]) \(\d+\) ")
CRASH_MARKERS = ("Guru Meditation", "abort()", "Backtrace:", "assert failed")
# The ROM prints "rst:0x1 (POWERON_RESET)" on every boot, so the first one
# is normal and each later one is a reboot.
RESET = "rst:"


def check(lines):
    """Return a dict answering the four questions for a list of log lines."""
    states = []
    calibration = []
    warnings = []
    crashes = []
    resets = 0
    last_counts = {}
    for raw in lines:
        line = raw.rstrip("\r\n")
        match = TRANSITION.search(line)
        if match:
            if not states:
                states.append(match.group(1))
            states.append(match.group(2))
        match = CAL_PEAK.search(line)
        if match:
            calibration.append(
                {
                    "tap": int(match.group(1)),
                    "value": float(match.group(2)),
                    "delay_ms": float(match.group(3)),
                }
            )
        if WARNING.match(line):
            warnings.append(line.strip())
        if any(marker in line for marker in CRASH_MARKERS):
            crashes.append(line.strip())
        if RESET in line:
            resets += 1
        fields = parse_line(line)
        if fields is not None:
            for key in ("under", "drop"):
                if isinstance(fields.get(key), (int, float)):
                    last_counts[key] = fields[key]
    return {
        "states": states,
        "calibration": calibration,
        "warnings": warnings,
        "crashes": crashes,
        "reboots": max(0, resets - 1),
        "under": last_counts.get("under"),
        "drop": last_counts.get("drop"),
    }


def problems(result):
    """Return the reasons the run needs a closer look (empty when it passed)."""
    reasons = []
    if "RUN" not in result["states"]:
        reasons.append("never reached RUN")
    if not result["calibration"]:
        reasons.append("no calibration peak line")
    if result["warnings"]:
        reasons.append(f"{len(result['warnings'])} warning or error line(s)")
    if result["under"] is None or result["drop"] is None:
        reasons.append("no telemetry line with under= and drop=")
    else:
        if result["under"] != 0:
            reasons.append(f"under={result['under']}")
        if result["drop"] != 0:
            reasons.append(f"drop={result['drop']}")
    if result["crashes"]:
        reasons.append("crash lines found")
    if result["reboots"]:
        reasons.append(f"rebooted {result['reboots']} time(s)")
    return reasons


def format_report(result):
    """Return the answers and the result as printable lines."""
    out = ["1. States reached: " + (" -> ".join(result["states"]) or "none found")]
    if result["calibration"]:
        for cal in result["calibration"]:
            out.append(
                f"2. Calibration: peak at tap {cal['tap']}, value {cal['value']:.4f}, "
                f"delay {cal['delay_ms']:.2f} ms"
            )
    else:
        out.append("2. Calibration: no 'peak at tap' line found")
    if result["warnings"]:
        out.append(f"   Warnings and errors ({len(result['warnings'])}):")
        out.extend(f"   {w}" for w in result["warnings"])
    else:
        out.append("   Warnings and errors: none")
    out.append(f"3. Last telemetry: under={result['under']} drop={result['drop']}")
    if result["crashes"] or result["reboots"]:
        out.append(f"4. Crashes: {len(result['crashes'])} line(s), reboots: {result['reboots']}")
        out.extend(f"   {c}" for c in result["crashes"])
    else:
        out.append("4. Crashes or reboots: none")
    reasons = problems(result)
    out.append("RESULT: PASS" if not reasons else "RESULT: CHECK (" + "; ".join(reasons) + ")")
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("path", help="saved idf.py monitor output")
    args = parser.parse_args()
    try:
        with open(args.path, encoding="utf-8", errors="replace") as log:
            result = check(log)
    except OSError as error:
        sys.exit(f"Cannot read log: {error}")
    print("\n".join(format_report(result)))
    sys.exit(0 if not problems(result) else 1)


if __name__ == "__main__":
    main()
