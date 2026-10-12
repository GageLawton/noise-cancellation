"""CLI coverage for selecting the number of survey peaks."""

import subprocess
import sys
import wave
from pathlib import Path

import pytest
from make_tone import make_tone

ANALYZE = Path(__file__).with_name("analyze.py")


@pytest.fixture
def recording(tmp_path):
    path = tmp_path / "tone.wav"
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(8000)
        wav.writeframes(make_tone(120.0, 3.0, 8000, 0.5).tobytes())
    return path


@pytest.mark.parametrize("options,count", [([], 8), (["--top", "3"], 3)])
def test_survey_lists_requested_number_of_peaks(recording, options, count):
    result = subprocess.run(
        [sys.executable, str(ANALYZE), str(recording), *options],
        capture_output=True, text=True, check=True,
    )
    heading = f"top {count} peaks in baseline:"
    assert "target frequency: 120.00 Hz" in result.stdout
    peaks = result.stdout.split(heading)[1].strip().splitlines()
    assert len(peaks) == count


@pytest.mark.parametrize("count", ["0", "-1", "abc"])
def test_survey_rejects_invalid_peak_counts(recording, count):
    result = subprocess.run(
        [sys.executable, str(ANALYZE), str(recording), "--top", count],
        capture_output=True, text=True, check=False,
    )
    assert result.returncode == 2
    assert "--top" in result.stderr


def test_survey_heading_matches_available_bins(recording):
    result = subprocess.run(
        [sys.executable, str(ANALYZE), str(recording), "--top", "10000"],
        capture_output=True, text=True, check=True,
    )
    # Inclusive 20..1000 Hz band, with 1/3 Hz bin spacing.
    heading = "top 2941 peaks in baseline:"
    assert len(result.stdout.split(heading)[1].strip().splitlines()) == 2941
