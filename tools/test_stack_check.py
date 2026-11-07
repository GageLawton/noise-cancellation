"""stack_check.py reports stack headroom and audio counter growth."""

import subprocess
import sys
from pathlib import Path

from stack_check import check, format_report, parse_stack_line, problems

TOOL = Path(__file__).with_name("stack_check.py")

GOOD = """\
I (1000) telemetry: up=1 state=RUN under=0 drop=0 filter=50 blocks=100
I (10000) telemetry: stack free: audio_io=1820 filter=2400 alignment=3100 config=900 telemetry=1500
I (20000) telemetry: stack free: audio_io=1800 filter=2380 alignment=3100 config=760 telemetry=1500
I (600000) telemetry: up=600 state=RUN under=0 drop=0 filter=30000 blocks=60000
"""

BAD = """\
I (1000) telemetry: up=1 state=RUN under=0 drop=0
I (10000) telemetry: stack free: audio_io=1820 filter=300 alignment=0 config=900 telemetry=1500
I (600000) telemetry: up=600 state=RUN under=4 drop=2
"""


def test_parse_stack_line():
    line = "I (1) telemetry: stack free: audio_io=10 filter=20"
    assert parse_stack_line(line) == {"audio_io": 10, "filter": 20}
    assert parse_stack_line("I (1) telemetry: up=1 filter=20") is None


def test_good_log_passes():
    result = check(GOOD.splitlines())
    assert result["stack_lines"] == 2
    assert result["smallest"]["config"] == 760
    assert result["smallest"]["audio_io"] == 1800
    # filter= on a normal telemetry line is a block count, not a stack size.
    assert result["smallest"]["filter"] == 2380
    assert result["growth"] == {"under": 0, "drop": 0}
    assert problems(result) == []
    assert format_report(result)[-1] == "RESULT: PASS"


def test_bad_log_lists_every_problem():
    reasons = problems(check(BAD.splitlines()))
    assert "filter has only 300 bytes free" in reasons
    assert "alignment never started (0 bytes)" in reasons
    assert "under grew by 4" in reasons
    assert "drop grew by 2" in reasons


def test_min_free_is_adjustable():
    result = check(GOOD.splitlines())
    assert "config has only 760 bytes free" in problems(result, min_free=1024)


def test_log_without_stack_lines():
    reasons = problems(check(["I (1) telemetry: under=0 drop=0"]))
    assert reasons == ["no 'stack free:' lines found"]


def test_cli(tmp_path):
    good = tmp_path / "good.log"
    good.write_text(GOOD)
    bad = tmp_path / "bad.log"
    bad.write_text(BAD)
    ok = subprocess.run([sys.executable, str(TOOL), str(good)],
                        capture_output=True, text=True, check=False)
    assert ok.returncode == 0
    assert "config: smallest 760 bytes free" in ok.stdout
    fail = subprocess.run([sys.executable, str(TOOL), str(bad)],
                          capture_output=True, text=True, check=False)
    assert fail.returncode == 1
    assert "<-- never started" in fail.stdout
    zero = subprocess.run([sys.executable, str(TOOL), str(good), "--min-free", "0"],
                          capture_output=True, text=True, check=False)
    assert zero.returncode != 0
    assert "--min-free must be 1 or more" in zero.stderr
