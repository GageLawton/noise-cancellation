"""spectrum.py turns a recording into levels per frequency."""

import numpy as np
import pytest
from make_tone import make_tone
from spectrum import spectrum_db

RATE = 8000


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
