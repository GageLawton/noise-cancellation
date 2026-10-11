from analyze import dominant_peak, narrowband_rms

import numpy as np


def test_dominant_peak_finds_tone():
    rate = 8000
    signal = np.sin(2 * np.pi * 120 * np.arange(3 * rate) / rate)

    assert abs(dominant_peak(signal, rate) - 120) < 0.5


def test_half_amplitude_is_6db_lower():
    rate = 8000
    signal = np.sin(2 * np.pi * 120 * np.arange(3 * rate) / rate)
    full_rms = narrowband_rms(signal, rate, 120)
    half_rms = narrowband_rms(signal / 2, rate, 120)

    difference_db = 20 * np.log10(full_rms / half_rms)
    assert abs(difference_db - 6.02) < 0.1
