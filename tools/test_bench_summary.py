"""bench_summary.py summarizes a telemetry log."""

import json
import subprocess
import sys
from pathlib import Path

import pytest
from bench_summary import dbfs_to_amplitude, format_summary, read_telemetry, summarize

TOOL = Path(__file__).with_name("bench_summary.py")

LOG = (
    "I (900) runtime: ANC node starting\n"
    "I (1000) telemetry: state=CALIBRATE_SECONDARY_PATH lock=0 clip=0 blocks=100 "
    "under=0 drop=0 mic=-50.0dBFS\n"
    "I (1500) runtime: CALIBRATE_SECONDARY_PATH -> RUN\n"
    "I (2000) telemetry: state=RUN lock=0 clip=0 blocks=350 under=0 drop=0 mic=-40.0dBFS "
    "resid=-30.0dBFS hum=-20.0dBFS\n"
    "I (3000) telemetry: state=RUN lock=1 clip=2 blocks=600 under=3 drop=0 mic=-30.0dBFS "
    "resid=-40.0dBFS hum=-22.0dBFS\n"
    "some noise the monitor printed\n"
    "I (4000) telemetry: state=RUN lock=1 clip=2 blocks=850 under=3 drop=1 mic=-45.0dBFS "
    "resid=-50.0dBFS hum=-24.0dBFS\n"
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


def test_tool_prints_json(tmp_path):
    log = tmp_path / "run.log"
    log.write_text(LOG, encoding="utf-8")
    result = subprocess.run(
        [sys.executable, str(TOOL), str(log), "--json"],
        capture_output=True,
        text=True,
        check=True,
    )
    summary = json.loads(result.stdout)
    assert summary["lines"] == 5
    assert summary["lock_fraction"] == pytest.approx(0.6)
    assert summary["counters"]["blocks"] == [100, 1100, 1000]


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


def test_counts_faults_and_sync_frames():
    log = [
        "I (1000) telemetry: state=RUN faults=0 syncrx=10 missed=0 bad=0",
        "I (2000) telemetry: state=RUN faults=1 syncrx=12 missed=1 bad=0",
    ]
    lines = format_summary(summarize(read_telemetry(log)))
    assert "faults: 0 -> 1 (+1)" in lines
    assert "syncrx: 10 -> 12 (+2)" in lines
    assert "missed: 0 -> 1 (+1)" in lines
    assert "bad: 0 -> 0 (+0)" in lines


def test_counts_sync_ticks_heard_and_sent():
    log = [
        "I (1000) telemetry: state=RUN sync=10/10",
        "I (2000) telemetry: state=RUN sync=11/12",
    ]
    lines = format_summary(summarize(read_telemetry(log)))
    assert "sync_heard: 10 -> 11 (+1)" in lines
    assert "sync_sent: 10 -> 12 (+2)" in lines


def test_resid_min_mean_max(summary):
    low, mean, high = summary["resid"]
    assert (low, high) == (-50.0, -30.0)
    assert mean == pytest.approx(-40.0)


def test_hum_min_mean_max(summary):
    low, mean, high = summary["hum"]
    assert (low, high) == (-24.0, -20.0)
    assert mean == pytest.approx(-22.0)


def test_levels_print_after_mic(summary):
    lines = format_summary(summary)
    assert "resid dBFS: min -50.0, mean -40.0, max -30.0" in lines
    assert "hum dBFS: min -24.0, mean -22.0, max -20.0" in lines


def test_log_without_resid_gives_none():
    records = read_telemetry(["I (1) telemetry: state=RUN mic=-40.0dBFS"])
    assert summarize(records)["resid"] is None


def test_clock_offset_range():
    records = read_telemetry(
        [
            "I (1) telemetry: state=RUN offset=-5us",
            "I (2) telemetry: state=RUN offset=-3us",
            "I (3) telemetry: state=RUN offset=-1us",
        ]
    )
    summary = summarize(records)
    assert summary["offset"] == (-5, -3, -1)
    assert any("clock offset" in line for line in format_summary(summary))


def test_hum_and_freq_split_by_lock(summary):
    # lock=1 lines have hum -22 and -24; the lock=0 RUN line has -20.
    assert summary["hum_locked"] == (-24.0, -23.0, -22.0)
    assert summary["hum_unlocked"] == (-20.0, -20.0, -20.0)
    # The sample log has no freq= field.
    assert summary["freq_locked"] is None


def test_freq_while_locked_ignores_unlocked_lines():
    records = read_telemetry([
        "I (1) telemetry: state=RUN lock=0 freq=0.00 hum=-40.0dBFS",
        "I (2) telemetry: state=RUN lock=1 freq=59.90 hum=-30.0dBFS",
        "I (3) telemetry: state=RUN lock=1 freq=60.10 hum=-30.0dBFS",
    ])
    result = summarize(records)
    assert result["freq_locked"] == pytest.approx((59.9, 60.0, 60.1))
    text = "\n".join(format_summary(result))
    assert "freq Hz while locked: min 59.90, mean 60.00, max 60.10" in text
    assert "hum dBFS while unlocked: min -40.0, mean -40.0, max -40.0" in text


def test_hum_amplitude_line(summary):
    text = "\n".join(format_summary(summary))
    # Mean hum is -22 dBFS, which is an amplitude of about 0.079.
    assert "hum amplitude: mean 0.079" in text
    assert dbfs_to_amplitude(-20.0) == pytest.approx(0.1)
