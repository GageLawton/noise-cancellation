"""telemetry_to_csv.py turns telemetry log lines into CSV rows."""

import csv
import subprocess
import sys
from pathlib import Path

from telemetry_to_csv import parse_line

TOOL = Path(__file__).with_name("telemetry_to_csv.py")


def test_typical_line_parses():
    line = "I (12345) telemetry: state=RUN conv=0 resid=-42.1dBFS blocks=2500 clip=0"
    assert parse_line(line) == {
        "state": "RUN",
        "conv": 0,
        "resid": -42.1,
        "blocks": 2500,
        "clip": 0,
    }


def test_other_lines_give_none():
    assert parse_line("I (1203) node: state CALIBRATE_SECONDARY_PATH") is None


def test_csv_header_has_every_key(tmp_path):
    log = tmp_path / "monitor.log"
    log.write_text(
        "I (1000) node: booting\n"
        "I (2000) telemetry: state=IDLE blocks=10\n"
        "I (3000) telemetry: state=RUN resid=-40.0dBFS\n",
        encoding="utf-8",
    )
    out = tmp_path / "telemetry.csv"

    subprocess.run([sys.executable, str(TOOL), str(log), "--out", str(out)], check=True)

    with out.open(newline="", encoding="utf-8") as f:
        rows = list(csv.reader(f))
    assert rows[0] == ["line", "state", "blocks", "resid"]
    assert rows[1] == ["2", "IDLE", "10", ""]
    assert rows[2] == ["3", "RUN", "", "-40.0"]
