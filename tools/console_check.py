#!/usr/bin/env python3
"""Check a saved serial-console log against the six console bench-check steps.

Open ``idf.py monitor``, type the commands from the serial-command bench
check (#254), save the whole session to a file, then run this tool on it.
The console echoes each typed line as ``> command``, so the tool can find
what you typed and what came back. It checks each step:

1. ``help`` printed the command list.
2. ``get output.ceiling``, ``set output.ceiling 0.5``, ``get output.ceiling``
   shows 0.5.
3. ``set output.ceiling 7`` was rejected.
4. ``recal`` answered and a new calibration peak line followed.
5. ``save`` succeeded, the board restarted, and ``get output.ceiling`` after
   the restart still shows 0.5.
6. ``set output.ceiling 0.8`` and ``save`` put it back.

It prints PASS or CHECK for each step and ends with ``RESULT: PASS`` or
``RESULT: CHECK``. The exit code is 0 for PASS and 1 for CHECK. Typed lines
the console didn't understand are listed too, since they are usually typos.

Usage:
    python tools/console_check.py console.log
"""

import argparse
import re
import sys

KEY = "output.ceiling"
SET_VALUE = 0.5
BAD_VALUE = "7"
RESTORE_VALUE = 0.8

# ESP-IDF log lines ("I (1234) tag: ...") are not console answers.
LOG_LINE = re.compile(r"^\s*[IWEDV] \(\d+\) ")
CAL_PEAK = re.compile(r"calibration: peak at tap \d+, value")
# The ROM prints "rst:" and the runtime prints "ANC node starting" on boot.
BOOT_MARKERS = ("rst:", "ANC node starting")
GET_ANSWER = re.compile(r"^(\S+) = (\S+)$")


def read_events(lines):
    """Turn log lines into ("cmd" | "answer" | "cal" | "boot", text) events."""
    events = []
    last_boot = False
    for raw in lines:
        line = raw.rstrip("\r\n")
        text = line.strip()
        if not text:
            continue
        if any(marker in line for marker in BOOT_MARKERS):
            # "rst:" and "ANC node starting" come from the same boot.
            if not last_boot:
                events.append(("boot", text))
            last_boot = True
            continue
        if CAL_PEAK.search(line):
            events.append(("cal", text))
        elif LOG_LINE.match(line):
            continue
        elif text.startswith("> "):
            events.append(("cmd", " ".join(text[2:].split())))
            last_boot = False
        else:
            events.append(("answer", text))
    return events


def commands(events):
    """Return [(index, command, [answer lines])] in the order they were typed."""
    found = []
    for i, (kind, text) in enumerate(events):
        if kind == "cmd":
            found.append((i, text, []))
        elif kind == "answer" and found:
            found[-1][2].append(text)
    return found


def get_value(answers):
    """Return the value from a 'key = value' answer, as a float when it is one."""
    for answer in answers:
        match = GET_ANSWER.match(answer)
        if match and match.group(1) == KEY:
            try:
                return float(match.group(2))
            except ValueError:
                return match.group(2)
    return None


def starts(answers, prefix):
    return any(a.startswith(prefix) for a in answers)


def after(events, start, kind):
    """Return the index of the first event of this kind after start, or None."""
    for i in range(start + 1, len(events)):
        if events[i][0] == kind:
            return i
    return None


def check(lines):
    """Return a list of (step number, title, passed, detail) for the six steps."""
    events = read_events(lines)
    cmds = commands(events)
    steps = []

    def first(pred, since=-1):
        for index, text, answers in cmds:
            if index > since and pred(text, answers):
                return index, text, answers
        return None

    # 1. help
    found = first(lambda t, a: t == "help")
    if found is None:
        steps.append((1, "help", False, "no '> help' line"))
    else:
        ok = starts(found[2], "commands:")
        steps.append((1, "help", ok, "printed the command list" if ok
                      else "no 'commands:' answer"))

    # 2. get, set 0.5, get
    set_cmd = f"set {KEY} {SET_VALUE:g}"
    before = first(lambda t, a: t == f"get {KEY}")
    set_found = first(lambda t, a: t == set_cmd)
    if set_found is None:
        steps.append((2, "get, set 0.5, get", False, f"no '> {set_cmd}' line"))
    else:
        detail = []
        if before is not None and before[0] < set_found[0]:
            detail.append(f"before: {KEY} = {get_value(before[2])}")
        ok = starts(set_found[2], "ok")
        if not ok:
            detail.append("set did not answer 'ok'")
        later = first(lambda t, a: t == f"get {KEY}", set_found[0])
        if later is None:
            ok = False
            detail.append(f"no 'get {KEY}' after the set")
        else:
            value = get_value(later[2])
            detail.append(f"after: {KEY} = {value}")
            ok = ok and value == SET_VALUE
        steps.append((2, "get, set 0.5, get", ok, "; ".join(detail)))

    # 3. set 7 is rejected
    bad_cmd = f"set {KEY} {BAD_VALUE}"
    found = first(lambda t, a: t == bad_cmd)
    if found is None:
        steps.append((3, "set 7 rejected", False, f"no '> {bad_cmd}' line"))
    else:
        ok = starts(found[2], "rejected")
        steps.append((3, "set 7 rejected", ok, "rejected" if ok
                      else "answer was: " + (" | ".join(found[2]) or "nothing")))

    # 4. recal answers and a calibration follows
    found = first(lambda t, a: t == "recal")
    if found is None:
        steps.append((4, "recal", False, "no '> recal' line"))
    else:
        answered = starts(found[2], "recalibrating")
        cal = after(events, found[0], "cal")
        ok = answered and cal is not None
        detail = "new calibration: " + events[cal][1] if cal is not None \
            else "no calibration peak line after recal"
        if not answered:
            detail = "no 'recalibrating' answer; " + detail
        steps.append((4, "recal", ok, detail))

    # 5. save, restart, get still shows 0.5
    save = first(lambda t, a: t == "save", set_found[0] if set_found else -1)
    if save is None:
        steps.append((5, "save survives a restart", False,
                      "no '> save' after the set to 0.5"))
    else:
        detail = []
        ok = starts(save[2], "saved;")
        if not ok:
            detail.append("save did not answer 'saved;'")
        boot = after(events, save[0], "boot")
        if boot is None:
            ok = False
            detail.append("no restart after the save")
        else:
            later = first(lambda t, a: t == f"get {KEY}", boot)
            if later is None:
                ok = False
                detail.append(f"no 'get {KEY}' after the restart")
            else:
                value = get_value(later[2])
                detail.append(f"after restart: {KEY} = {value}")
                ok = ok and value == SET_VALUE
        steps.append((5, "save survives a restart", ok, "; ".join(detail)))

    # 6. set 0.8 and save
    restore_cmd = f"set {KEY} {RESTORE_VALUE:g}"
    found = first(lambda t, a: t == restore_cmd)
    if found is None:
        steps.append((6, "put 0.8 back", False, f"no '> {restore_cmd}' line"))
    else:
        ok = starts(found[2], "ok")
        save = first(lambda t, a: t == "save", found[0])
        saved = save is not None and starts(save[2], "saved;")
        detail = []
        if not ok:
            detail.append("set did not answer 'ok'")
        detail.append("saved" if saved else "no successful save after it")
        steps.append((6, "put 0.8 back", ok and saved, "; ".join(detail)))

    return steps


def not_understood(lines):
    """Return the typed lines the console answered with 'unknown command' or similar."""
    out = []
    for _, text, answers in commands(read_events(lines)):
        if starts(answers, "unknown command") or starts(answers, "line too long"):
            out.append(text)
    return out


def format_report(steps, typos=()):
    out = []
    for number, title, ok, detail in steps:
        out.append(f"{number}. {title}: {'PASS' if ok else 'CHECK'} ({detail})")
    if typos:
        out.append("Not understood: " + ", ".join(f"'{t}'" for t in typos))
    failed = [str(n) for n, _, ok, _ in steps if not ok]
    out.append("RESULT: PASS" if not failed
               else "RESULT: CHECK (step " + ", ".join(failed) + ")")
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("path", help="saved idf.py monitor output")
    args = parser.parse_args()
    try:
        with open(args.path, encoding="utf-8", errors="replace") as log:
            lines = log.readlines()
    except OSError as error:
        sys.exit(f"Cannot read log: {error}")
    steps = check(lines)
    print("\n".join(format_report(steps, not_understood(lines))))
    sys.exit(0 if all(ok for _, _, ok, _ in steps) else 1)


if __name__ == "__main__":
    main()
