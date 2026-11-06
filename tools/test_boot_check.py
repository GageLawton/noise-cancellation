"""boot_check.py answers the first-boot bench-check questions."""

import subprocess
import sys
from pathlib import Path

from boot_check import check, format_report, problems

TOOL = Path(__file__).with_name("boot_check.py")

GOOD = """\
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
I (300) runtime: ANC node starting
I (310) runtime: BOOT -> INIT_HAL
I (330) runtime: INIT_HAL -> CALIBRATE_SECONDARY_PATH
I (900) runtime: calibration: peak at tap 171, value 0.4200, delay 10.69 ms
I (950) runtime: CALIBRATE_SECONDARY_PATH -> RUN
I (1000) telemetry: up=1 state=RUN lock=0 under=0 drop=0 mic=-40.0dBFS
I (2000) telemetry: up=2 state=RUN lock=1 under=0 drop=0 mic=-41.0dBFS
"""

BAD = """\
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
I (310) runtime: BOOT -> INIT_HAL
I (330) runtime: INIT_HAL -> CALIBRATE_SECONDARY_PATH
W (900) runtime: calibration: no clear peak -- is the speaker playing and the mic connected?
I (1000) telemetry: up=1 state=CALIBRATE_SECONDARY_PATH under=2 drop=1
Guru Meditation Error: Core  1 panic'ed (LoadProhibited).
Backtrace: 0x40081234:0x3ffb0000
rst:0xc (RTC_SW_CPU_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
"""


def test_good_log_passes():
    result = check(GOOD.splitlines())
    assert result["states"] == [
        "BOOT", "INIT_HAL", "CALIBRATE_SECONDARY_PATH", "RUN",
    ]
    assert result["calibration"] == [{"tap": 171, "value": 0.42, "delay_ms": 10.69}]
    assert result["under"] == 0 and result["drop"] == 0
    assert result["reboots"] == 0
    assert problems(result) == []
    assert format_report(result)[-1] == "RESULT: PASS"


def test_bad_log_lists_every_problem():
    result = check(BAD.splitlines())
    assert "RUN" not in result["states"]
    assert result["calibration"] == []
    assert len(result["warnings"]) == 1
    assert len(result["crashes"]) == 2
    assert result["reboots"] == 1
    reasons = problems(result)
    assert "never reached RUN" in reasons
    assert "under=2" in reasons and "drop=1" in reasons
    assert "rebooted 1 time(s)" in reasons
    assert format_report(result)[-1].startswith("RESULT: CHECK (")


def test_log_without_telemetry_needs_a_check():
    result = check(["I (310) runtime: BOOT -> INIT_HAL"])
    assert "no telemetry line with under= and drop=" in problems(result)


def test_cli_exit_codes(tmp_path):
    good = tmp_path / "good.log"
    good.write_text(GOOD)
    bad = tmp_path / "bad.log"
    bad.write_text(BAD)
    ok = subprocess.run([sys.executable, str(TOOL), str(good)],
                        capture_output=True, text=True, check=False)
    assert ok.returncode == 0
    assert "RESULT: PASS" in ok.stdout
    fail = subprocess.run([sys.executable, str(TOOL), str(bad)],
                          capture_output=True, text=True, check=False)
    assert fail.returncode == 1
    assert "RESULT: CHECK" in fail.stdout


def test_cli_missing_file(tmp_path):
    run = subprocess.run([sys.executable, str(TOOL), str(tmp_path / "nope.log")],
                         capture_output=True, text=True, check=False)
    assert run.returncode != 0
    assert "Cannot read log" in run.stderr
