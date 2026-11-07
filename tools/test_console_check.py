"""console_check.py checks the six serial-console bench-check steps."""

import subprocess
import sys
from pathlib import Path

from console_check import check, format_report, not_understood

TOOL = Path(__file__).with_name("console_check.py")

GOOD = """\
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
I (300) runtime: ANC node starting
I (950) runtime: CALIBRATE_SECONDARY_PATH -> RUN
> help
commands:
  get <key>          print a setting
  set <key> <value>  change it (applies at the next calibration)
> get output.ceiling
output.ceiling = 0.8
> set output.ceiling 0.5
ok, applies at the next calibration (type recal to start one now)
I (5000) telemetry: up=5 state=RUN under=0 drop=0
> get output.ceiling
output.ceiling = 0.5
> set output.ceiling 7
rejected: unknown key, or value out of range
> recal
recalibrating
I (9000) runtime: RUN -> RECHECK_SECONDARY_PATH
I (9500) runtime: calibration: peak at tap 171, value 0.4200, delay 10.69 ms
> save
saved; recalibrating, since the flash write paused the audio
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
I (300) runtime: ANC node starting
> get output.ceiling
output.ceiling = 0.5
> set output.ceiling 0.8
ok, applies at the next calibration (type recal to start one now)
> save
saved; recalibrating, since the flash write paused the audio
"""


def results(text):
    return {n: ok for n, _, ok, _ in check(text.splitlines())}


def test_good_log_passes_every_step():
    assert results(GOOD) == {n: True for n in range(1, 7)}
    report = format_report(check(GOOD.splitlines()))
    assert report[-1] == "RESULT: PASS"
    assert "before: output.ceiling = 0.8" in report[1]


def test_value_lost_after_restart_fails_step_5():
    bad = GOOD.replace(
        "ANC node starting\n> get output.ceiling\noutput.ceiling = 0.5",
        "ANC node starting\n> get output.ceiling\noutput.ceiling = 0.8",
    )
    got = results(bad)
    assert got[5] is False
    assert all(got[n] for n in (1, 2, 3, 4, 6))


def test_no_restart_fails_step_5():
    bad = GOOD.replace(
        "rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)\nI (300) runtime: ANC node starting\n> get",
        "> get",
    )
    assert results(bad)[5] is False


def test_set_7_accepted_fails_step_3():
    bad = GOOD.replace(
        "> set output.ceiling 7\nrejected: unknown key, or value out of range",
        "> set output.ceiling 7\nok, applies at the next calibration",
    )
    assert results(bad)[3] is False


def test_recal_without_calibration_fails_step_4():
    bad = GOOD.replace(
        "I (9500) runtime: calibration: peak at tap 171, value 0.4200, delay 10.69 ms\n", ""
    )
    assert results(bad)[4] is False


def test_empty_log_fails_everything():
    steps = check([])
    assert [ok for _, _, ok, _ in steps] == [False] * 6
    assert format_report(steps)[-1].startswith("RESULT: CHECK")


def test_typos_are_listed():
    log = GOOD + "> hlep\nunknown command, try help\n"
    assert not_understood(log.splitlines()) == ["hlep"]


def test_command_line_exit_codes(tmp_path):
    good = tmp_path / "good.log"
    good.write_text(GOOD)
    run = subprocess.run([sys.executable, str(TOOL), str(good)],
                         capture_output=True, text=True, check=False)
    assert run.returncode == 0
    assert "RESULT: PASS" in run.stdout
    empty = tmp_path / "empty.log"
    empty.write_text("")
    run = subprocess.run([sys.executable, str(TOOL), str(empty)],
                         capture_output=True, text=True, check=False)
    assert run.returncode == 1
