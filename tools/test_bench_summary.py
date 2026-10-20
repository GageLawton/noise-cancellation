"""bench_summary.py summarizes a telemetry log."""

import subprocess
import sys
from pathlib import Path

import pytest
from bench_summary import read_telemetry, summarize

TOOL = Path(__file__).with_name("bench_summary.py")

LOG = (
    "I (900) runtime: ANC node starting\n"
    "I (1000) telemetry: state=CALIBRATE_SECONDARY_PATH lock=0 clip=0 blocks=100 "
    "under=0 drop=0 mic=-50.0dBFS\n"
    "I (1500) runtime: CALIBRATE_SECONDARY_PATH -> RUN\n"
    "I (2000) telemetry: state=RUN lock=0 clip=0 blocks=350 under=0 drop=0 mic=-40.0dBFS\n"
    "I (3000) telemetry: state=RUN lock=1 clip=2 blocks=600 under=3 drop=0 mic=-30.0dBFS\n"
    "some noise the monitor printed\n"
    "I (4000) telemetry: state=RUN lock=1 clip=2 blocks=850 under=3 drop=1 mic=-45.0dBFS\n"
    "I (5000) telemetry: state=FAULT lock=1 clip=5 blocks=1100 under=3 drop=1 "
    "mic=-35.0dBFS\n"
)


@pytest.fixture
def summary():
    return summarize(read_telemetry(LOG.splitlines()))


def test_counts_telemetry_lines(summary):
    assert summary["lines"] == 5
    assert summary["seconds"] == 5


def test_counters_first_last_and_growth(summary):
    assert summary["counters"]["blocks"] == (100, 1100, 1000)
    assert summary["counters"]["under"] == (0, 3, 3)
    assert summary["counters"]["drop"] == (0, 1, 1)
    assert summary["counters"]["clip"] == (0, 5, 5)


def test_mic_min_mean_max(summary):
    low, mean, high = summary["mic"]
    assert low == -50.0
    assert mean == pytest.approx(-40.0)
    assert high == -30.0


def test_lock_fraction(summary):
    assert summary["lock_fraction"] == pytest.approx(0.6)


def test_states_in_order_of_first_appearance(summary):
    assert summary["states"] == ["CALIBRATE_SECONDARY_PATH", "RUN", "FAULT"]


def test_tool_prints_summary(tmp_path):
    log = tmp_path / "run.log"
    log.write_text(LOG, encoding="utf-8")
    result = subprocess.run(
        [sys.executable, str(TOOL), str(log)],
        capture_output=True,
        text=True,
        check=True,
    )
    assert "telemetry lines: 5" in result.stdout
    assert "blocks: 100 -> 1100 (+1000)" in result.stdout
    assert "locked: 60% of lines" in result.stdout


def test_tool_exits_without_telemetry(tmp_path):
    log = tmp_path / "empty.log"
    log.write_text("I (1) runtime: ANC node starting\n", encoding="utf-8")
    result = subprocess.run(
        [sys.executable, str(TOOL), str(log)],
        check=False,
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "No telemetry" in result.stderr
