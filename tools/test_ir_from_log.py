"""ir_from_log.py finds impulse response taps inside noisy serial logs."""

import subprocess
import sys
from pathlib import Path

import pytest
from ir_from_log import find_peak, missing_indices, parse_ir

LOG = """\
I (1203) node: state CALIBRATE_SECONDARY_PATH
I (1530) cal: ir,2,-0.0125
some unrelated line, with commas 1,2,3
ir,0,0.0010
I (1531) cal: ir,1,0.4310
W (1600) node: telemetry mic=-42.0 dBFS
"""


def test_parses_out_of_order_taps_among_noise():
    assert parse_ir(LOG.splitlines()) == pytest.approx([0.001, 0.431, -0.0125])


def test_dropped_line_keeps_later_taps_in_place():
    taps = parse_ir(["ir,0,0.1", "ir,2,0.9"])
    assert taps == pytest.approx([0.1, 0.0, 0.9])
    assert find_peak(taps) == (2, pytest.approx(0.9))


def test_words_ending_in_ir_are_not_taps():
    assert parse_ir(["fir,3,0.5", "pair,1,0.2", "I (5) cal: ir,0,0.25"]) == [0.25]


def test_missing_indices():
    assert missing_indices({0: 0.1, 3: 0.2}) == [1, 2]
    assert missing_indices({}) == []


def test_empty_input_gives_empty_list():
    assert parse_ir([]) == []


def test_accepts_exponent_values():
    assert parse_ir(["ir,0,1.5e-3", "ir,1,-2E2"]) == pytest.approx([0.0015, -200.0])


def test_peak_keeps_its_sign():
    assert find_peak([0.0, 0.1, -0.7, 0.3]) == (2, -0.7)


def test_main_prints_summary_and_writes_csv(tmp_path):
    log = tmp_path / "monitor.log"
    log.write_text(LOG)
    out = tmp_path / "ir.csv"
    script = Path(__file__).with_name("ir_from_log.py")
    result = subprocess.run(
        [sys.executable, str(script), str(log), "--csv", str(out)],
        capture_output=True,
        text=True,
        check=True,
    )
    assert "taps:  3" in result.stdout
    assert "index 1" in result.stdout
    assert "0.125 ms" in result.stdout
    assert out.read_text().splitlines() == [
        "index,value",
        "0,0.001",
        "1,0.431",
        "2,-0.0125",
    ]


def test_main_warns_about_missing_taps(tmp_path):
    log = tmp_path / "gappy.log"
    log.write_text("ir,0,0.1\nir,2,0.9\n")
    script = Path(__file__).with_name("ir_from_log.py")
    result = subprocess.run(
        [sys.executable, str(script), str(log)],
        capture_output=True,
        text=True,
        check=True,
    )
    assert "taps:  3" in result.stdout
    assert "index 2" in result.stdout
    assert "1 taps missing" in result.stderr


def test_main_fails_without_ir_lines(tmp_path):
    log = tmp_path / "empty.log"
    log.write_text("nothing here\n")
    script = Path(__file__).with_name("ir_from_log.py")
    result = subprocess.run(
        [sys.executable, str(script), str(log)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode != 0
    assert "No ir" in result.stderr
