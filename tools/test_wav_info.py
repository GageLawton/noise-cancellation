"""Known-level recordings validate WAV metadata and multichannel levels."""

import math
import subprocess
import sys
import wave
from pathlib import Path

import numpy as np
import pytest
from make_tone import make_tone
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
        "clipped:     0 samples",
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


def test_cli_json_output(tmp_path):
    import json

    path = tmp_path / "half.wav"
    write_wav(path, np.full(8000, 16384, dtype="<i2").tobytes())

    result = subprocess.run(
        [
            sys.executable,
            str(Path(__file__).with_name("wav_info.py")),
            str(path),
            "--json",
        ],
        check=True,
        capture_output=True,
        text=True,
    )

    info = json.loads(result.stdout)
    assert info["sample_rate"] == 8000


def test_cli_json_silence_uses_null(tmp_path):
    import json

    path = tmp_path / "silence.wav"
    write_wav(path, np.zeros(8000, dtype="<i2").tobytes())

    result = subprocess.run(
        [
            sys.executable,
            str(Path(__file__).with_name("wav_info.py")),
            str(path),
            "--json",
        ],
        check=True,
        capture_output=True,
        text=True,
    )

    info = json.loads(result.stdout)
    assert info["peak_dbfs"] is None


def test_stereo_levels_per_channel(tmp_path):
    import json

    path = tmp_path / "left-tone.wav"
    # Half-scale tone on the left, silence on the right.
    write_wav(path, make_tone(120.0, 1.0, 8000, 0.5, stereo=True).tobytes(), channels=2)

    left, right = summarize(path)["channels_dbfs"]
    assert left["peak_dbfs"] == pytest.approx(20 * math.log10(0.5), abs=0.01)
    assert left["rms_dbfs"] == pytest.approx(20 * math.log10(0.5 / math.sqrt(2)), abs=0.01)
    assert right == {"peak_dbfs": -math.inf, "rms_dbfs": -math.inf}

    script = str(Path(__file__).with_name("wav_info.py"))
    text = subprocess.run(
        [sys.executable, script, str(path)], check=True, capture_output=True, text=True
    ).stdout.splitlines()
    assert text[-2:] == [
        "channel 1:   peak -6.0 dBFS, rms -9.0 dBFS",
        "channel 2:   peak -inf dBFS, rms -inf dBFS",
    ]

    result = subprocess.run(
        [sys.executable, script, str(path), "--json"],
        check=True,
        capture_output=True,
        text=True,
    )
    assert json.loads(result.stdout)["channels_dbfs"][1] == {
        "peak_dbfs": None,
        "rms_dbfs": None,
    }


def test_half_scale_tone_has_no_clipped_samples(tmp_path):
    path = tmp_path / "half-tone.wav"
    write_wav(path, make_tone(120.0, 1.0, 8000, 0.5).tobytes())
    assert summarize(path)["clipped_samples"] == 0


def test_clipped_samples_are_counted(tmp_path):
    import json

    path = tmp_path / "clipped.wav"
    samples = make_tone(120.0, 1.0, 8000, 0.5)
    samples[[10, 200, 3000]] = 32767
    write_wav(path, samples.tobytes())
    assert summarize(path)["clipped_samples"] == 3

    result = subprocess.run(
        [sys.executable, str(Path(__file__).with_name("wav_info.py")), str(path), "--json"],
        check=True,
        capture_output=True,
        text=True,
    )
    assert json.loads(result.stdout)["clipped_samples"] == 3
