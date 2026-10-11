"""Unit tests for make_tone.py."""

import numpy as np
import pytest
from analyze import dominant_peak
from make_tone import make_tone

RATE = 8000
FREQ = 120.0
SECONDS = 3.0
AMPLITUDE = 0.5


def test_no_noise_matches_original_output():
    """The default output must stay identical to the original implementation."""
    t = np.arange(round(SECONDS * RATE), dtype=np.float64) / RATE
    tone = AMPLITUDE * np.sin(2.0 * np.pi * FREQ * t)
    expected = np.round(tone * 32767.0).astype(np.int16)

    actual = make_tone(FREQ, SECONDS, RATE, AMPLITUDE)

    assert actual.tobytes() == expected.tobytes()


def test_noise_is_repeatable():
    """The fixed random seed should produce identical output every run."""
    first = make_tone(FREQ, SECONDS, RATE, AMPLITUDE, noise=0.2)
    second = make_tone(FREQ, SECONDS, RATE, AMPLITUDE, noise=0.2)

    assert np.array_equal(first, second)


def test_noisy_tone_still_has_correct_frequency():
    samples = make_tone(FREQ, SECONDS, RATE, AMPLITUDE, noise=0.2)
    signal = samples.astype(np.float64) / 32768.0

    assert dominant_peak(signal, RATE) == pytest.approx(FREQ, abs=0.5)
