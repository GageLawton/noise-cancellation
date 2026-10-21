#!/usr/bin/env python3
"""Save a serial log with the time each line arrived.

Bench checks need saved logs. ``idf.py monitor | tee`` works, but loses
when each line came in. This writes every line to a file prefixed with the
seconds since the capture started (``12.345 I (1234) anc: ...``), echoes it
to the screen, and stops after --seconds or on Ctrl+C.

The port name depends on your computer:
    Windows   COM5 (see Device Manager, Ports)
    Linux     /dev/ttyUSB0
    macOS     /dev/cu.usbserial-*

Needs pyserial, which only bench users need, so it isn't in
requirements.txt:
    pip install pyserial

Usage:
    python tools/capture.py /dev/ttyUSB0 --out run.log
    python tools/capture.py COM5 --out run.log --seconds 60
"""

import argparse
import sys
import time


def stamp(line_bytes, elapsed_s):
    """Return one serial line as text, prefixed with elapsed_s.

    Bytes that aren't valid UTF-8 become U+FFFD rather than stopping the
    capture, and the line ending is stripped.
    """
    text = line_bytes.decode("utf-8", errors="replace").rstrip("\r\n")
    return f"{elapsed_s:.3f} {text}"


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("port", help="serial port, for example COM5 or /dev/ttyUSB0")
    p.add_argument("--out", required=True, help="path of the log file to write")
    p.add_argument("--baud", type=int, default=115200,
                   help="baud rate (default: 115200)")
    p.add_argument("--seconds", type=float,
                   help="stop after this many seconds (default: run until Ctrl+C)")
    args = p.parse_args()

    if args.baud <= 0:
        p.error("--baud must be greater than 0")
    if args.seconds is not None and args.seconds <= 0:
        p.error("--seconds must be greater than 0")

    try:
        import serial
    except ImportError:
        sys.exit("capture.py needs pyserial: pip install pyserial")

    try:
        # A short timeout lets the loop notice --seconds even when the
        # board is quiet.
        port = serial.Serial(args.port, args.baud, timeout=0.2)
    except serial.SerialException as e:
        sys.exit(f"cannot open {args.port}: {e}")

    start = time.monotonic()
    lines = 0
    with port, open(args.out, "w", encoding="utf-8") as out:
        try:
            while args.seconds is None or time.monotonic() - start < args.seconds:
                raw = port.readline()
                if not raw:
                    continue
                line = stamp(raw, time.monotonic() - start)
                out.write(line + "\n")
                out.flush()
                print(line)
                lines += 1
        except KeyboardInterrupt:
            pass
    print(f"{args.out}: {lines} lines", file=sys.stderr)


if __name__ == "__main__":
    main()
