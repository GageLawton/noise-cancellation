"""Known-level recordings validate WAV metadata and multichannel levels."""

import math
import subprocess
import sys
import wave
from pathlib import Path

import numpy as np
import pytest
from wav_info import summarize


def write_wav(path, samples, channels=1, width=2):
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(channels)
        wav.setsampwidth(width)
        wav.setframerate(8000)
        wav.writeframes(samples)


def test_full_scale_square_wave(tmp_path):
    path = tmp_path / "square.wav"
    samples = np.tile(np.array([32767, -32767], dtype="<i2"), 4000)
    write_wav(path, samples.tobytes())
    info = summarize(path)
    assert info["channels"] == 1
    assert info["sample_rate"] == 8000
    assert info["duration"] == 1.0
    assert info["peak_dbfs"] == pytest.approx(0.0, abs=0.001)
    assert info["rms_dbfs"] == pytest.approx(0.0, abs=0.001)


def test_levels_include_all_channels(tmp_path):
    path = tmp_path / "stereo.wav"
    # Silent left channel; half-scale right channel. RMS averages both.
    samples = np.tile(np.array([0, -16384], dtype="<i2"), 8000)
    write_wav(path, samples.tobytes(), channels=2)
    info = summarize(path)
    assert info["channels"] == 2
    assert info["duration"] == 1.0
    assert info["peak_dbfs"] == pytest.approx(20 * math.log10(0.5))
    assert info["rms_dbfs"] == pytest.approx(20 * math.log10(0.5 / math.sqrt(2)))


@pytest.mark.parametrize("frames", [0, 8000])
def test_silence_has_negative_infinite_levels(tmp_path, frames):
    path = tmp_path / "silence.wav"
    write_wav(path, np.zeros(frames, dtype="<i2").tobytes())
    info = summarize(path)
    assert info["duration"] == frames / 8000
    assert info["peak_dbfs"] == -math.inf
    assert info["rms_dbfs"] == -math.inf


def test_minimum_int16_does_not_overflow(tmp_path):
    path = tmp_path / "minimum.wav"
    write_wav(path, np.array([-32768], dtype="<i2").tobytes())
    info = summarize(path)
    assert info["peak_dbfs"] == 0.0
    assert info["rms_dbfs"] == 0.0


def test_non_16_bit_recording_is_rejected(tmp_path):
    path = tmp_path / "eight-bit.wav"
    write_wav(path, b"\x80" * 8000, width=1)
    with pytest.raises(ValueError, match="expected 16-bit PCM"):
        summarize(path)


def test_cli_prints_metadata_and_levels(tmp_path):
    path = tmp_path / "half.wav"
    write_wav(path, np.full(8000, 16384, dtype="<i2").tobytes())
    result = subprocess.run(
        [sys.executable, str(Path(__file__).with_name("wav_info.py")), str(path)],
        check=True, capture_output=True, text=True,
    )
    assert result.stdout.splitlines() == [
        f"file:        {path}", "channels:    1", "sample rate: 8000 Hz",
        "duration:    1.00 s", "peak:        -6.0 dBFS", "rms:         -6.0 dBFS",
    ]


def test_cli_reports_unsupported_width_without_traceback(tmp_path):
    path = tmp_path / "eight-bit.wav"
    write_wav(path, b"\x80" * 8000, width=1)
    result = subprocess.run(
        [sys.executable, str(Path(__file__).with_name("wav_info.py")), str(path)],
        check=False, capture_output=True, text=True,
    )
    assert result.returncode != 0
    assert "expected 16-bit PCM" in result.stderr
    assert "Traceback" not in result.stderr
