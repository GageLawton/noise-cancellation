"""Unit tests for make_tone.py."""

import numpy as np
import pytest
from analyze import dominant_peak
from make_tone import make_sweep, make_tone

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


def test_harmonics_add_peaks_at_multiples():
    samples = make_tone(60.0, SECONDS, RATE, AMPLITUDE, harmonics=2)
    spectrum = np.abs(np.fft.rfft(samples.astype(np.float64)))
    freqs = np.fft.rfftfreq(samples.size, 1.0 / RATE)

    def height(hz):
        return spectrum[np.argmin(np.abs(freqs - hz))]

    # The three strongest bins are the tone and its two harmonics.
    top = sorted(freqs[np.argsort(spectrum)[-3:]])
    assert top == pytest.approx([60.0, 120.0, 180.0], abs=0.5)
    # Harmonic k has amplitude / k, so 120 Hz is half as tall as 60 Hz.
    assert height(120.0) / height(60.0) == pytest.approx(0.5, abs=0.02)


def strongest_hz(samples, rate):
    spectrum = np.abs(np.fft.rfft(samples.astype(np.float64)))
    return np.fft.rfftfreq(samples.size, 1.0 / rate)[np.argmax(spectrum)]


def test_sweep_glides_from_start_to_end():
    samples = make_sweep(100.0, 200.0, 1.0, RATE, AMPLITUDE)
    tenth = RATE // 10

    assert samples.size == RATE
    assert samples.dtype == np.int16
    # Over the first and last 0.1 s the frequency averages 105 Hz and 195 Hz.
    assert strongest_hz(samples[:tenth], RATE) == pytest.approx(105.0, abs=10.0)
    assert strongest_hz(samples[-tenth:], RATE) == pytest.approx(195.0, abs=10.0)
