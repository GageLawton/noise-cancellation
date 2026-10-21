"""telemetry_to_csv.py turns telemetry log lines into CSV rows."""

import csv
import subprocess
import sys
from pathlib import Path

import pytest
from telemetry_to_csv import parse_line, select_columns, select_rows

TOOL = Path(__file__).with_name("telemetry_to_csv.py")

LOG = (
    "I (1000) node: booting\n"
    "I (2000) telemetry: state=IDLE blocks=10\n"
    "I (3000) telemetry: state=RUN resid=-40.0dBFS\n"
)


def run_tool(tmp_path, *extra):
    """Runs the tool on LOG and returns the CSV rows it wrote."""
    log = tmp_path / "monitor.log"
    log.write_text(LOG, encoding="utf-8")
    out = tmp_path / "telemetry.csv"
    subprocess.run(
        [sys.executable, str(TOOL), str(log), "--out", str(out), *extra], check=True
    )
    with out.open(newline="", encoding="utf-8") as f:
        return list(csv.reader(f))


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
    rows = run_tool(tmp_path)
    assert rows[0] == ["line", "state", "blocks", "resid"]
    assert rows[1] == ["2", "IDLE", "10", ""]
    assert rows[2] == ["3", "RUN", "", "-40.0"]


def test_columns_keeps_only_the_keys_asked_for(tmp_path):
    rows = run_tool(tmp_path, "--columns", "resid,state")
    assert rows[0] == ["line", "resid", "state"]
    assert rows[1] == ["2", "", "IDLE"]
    assert rows[2] == ["3", "-40.0", "RUN"]


def test_select_columns_keeps_requested_order():
    assert select_columns(["line", "state", "blocks", "resid"], "resid,state") == [
        "line",
        "resid",
        "state",
    ]


def test_unknown_column_exits_with_its_name():
    with pytest.raises(SystemExit) as error:
        select_columns(["line", "state"], "state,volume")
    assert "volume" in str(error.value)


def test_state_keeps_only_matching_rows(tmp_path):
    rows = run_tool(tmp_path, "--state", "RUN")
    assert rows[0] == ["line", "state", "blocks", "resid"]
    assert rows[1:] == [["3", "RUN", "", "-40.0"]]


def test_unknown_state_exits_with_message():
    with pytest.raises(SystemExit) as error:
        select_rows([{"line": 2, "state": "IDLE"}], "RUN")
    assert str(error.value) == "No telemetry lines with state=RUN"
