"""spectrum.py turns a recording into levels per frequency."""

import csv
import subprocess
import sys
import wave
from pathlib import Path

import numpy as np
import pytest
from make_tone import make_tone
from spectrum import spectrum_db

RATE = 8000
TOOL = Path(__file__).with_name("spectrum.py")


def test_tone_peaks_at_its_frequency():
    signal = make_tone(120.0, 3.0, RATE, 0.5).astype(np.float64) / 32768.0
    freqs, levels = spectrum_db(signal, RATE)
    assert freqs[np.argmax(levels)] == pytest.approx(120.0, abs=2.0)
    # Half scale is about -6 dB relative to full scale.
    assert np.max(levels) == pytest.approx(-6.0, abs=0.1)


def test_silence_gives_finite_levels():
    freqs, levels = spectrum_db(np.zeros(RATE), RATE)
    assert len(freqs) == len(levels)
    assert np.all(np.isfinite(levels))


def test_tool_writes_csv(tmp_path):
    wav = tmp_path / "tone.wav"
    with wave.open(str(wav), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(make_tone(120.0, 3.0, RATE, 0.5).tobytes())
    out = tmp_path / "levels.csv"
    # Works whether or not matplotlib is installed; the PNG goes to tmp_path.
    subprocess.run(
        [sys.executable, str(TOOL), str(wav), "--csv", str(out),
         "--out", str(tmp_path / "spectrum.png")],
        capture_output=True,
        text=True,
        check=True,
    )
    with open(out, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    assert rows
    assert all(float(r["freq_hz"]) <= 1000.0 for r in rows)
    loudest = max(rows, key=lambda r: float(r["level_db"]))
    assert float(loudest["freq_hz"]) == pytest.approx(120.0, abs=2.0)
