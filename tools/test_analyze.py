"""Unit tests for the offline analysis helpers.

Synthetic numpy signals only -- no WAV files and no hardware, so these
run anywhere `pytest tools` does.
"""

import math
import wave

import numpy as np
import pytest

from analyze import dominant_peak, narrowband_rms, read_wav
from make_tone import make_tone

RATE = 8000
FREQ = 120.0
SECONDS = 3.0


def sine(freq_hz=FREQ, rate=RATE, seconds=SECONDS, amplitude=1.0):
    t = np.arange(int(rate * seconds)) / rate
    return amplitude * np.sin(2.0 * np.pi * freq_hz * t)


def test_dominant_peak_finds_tone():
    assert dominant_peak(sine(), RATE) == pytest.approx(FREQ, abs=0.5)


def test_dominant_peak_ignores_tones_outside_the_range():
    # A loud 3 kHz tone must not win when the search range stops at 1 kHz.
    signal = sine(amplitude=0.1) + sine(freq_hz=3000.0, amplitude=1.0)
    assert dominant_peak(signal, RATE, lo=20.0, hi=1000.0) == pytest.approx(FREQ, abs=0.5)


def test_half_amplitude_is_6db_lower():
    full = narrowband_rms(sine(amplitude=1.0), RATE, FREQ)
    half = narrowband_rms(sine(amplitude=0.5), RATE, FREQ)
    assert 20.0 * math.log10(full / half) == pytest.approx(6.02, abs=0.1)


def test_narrowband_rms_rejects_an_out_of_band_tone():
    """Energy 200 Hz away should barely register in a +/-2 Hz band."""
    in_band = narrowband_rms(sine(), RATE, FREQ)
    off_band = narrowband_rms(sine(freq_hz=320.0), RATE, FREQ)
    assert off_band < in_band / 100.0


def test_narrowband_rms_exits_when_no_bin_matches():
    # 10 ms of audio gives 100 Hz bin spacing, so nothing lands within
    # +/-2 Hz of 120 Hz and the function should bail out rather than
    # silently report zero.
    with pytest.raises(SystemExit):
        narrowband_rms(sine(seconds=0.01), RATE, FREQ)


def write_wav(path, samples, channels, rate=RATE):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(channels)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(samples.tobytes())


def test_read_wav_mono_round_trip(tmp_path):
    path = tmp_path / "mono.wav"
    write_wav(path, make_tone(FREQ, SECONDS, RATE, 0.5), channels=1)
    data, rate = read_wav(str(path))
    assert rate == RATE
    assert len(data) == int(RATE * SECONDS)
    assert dominant_peak(data, rate) == pytest.approx(FREQ, abs=0.5)


def test_read_wav_stereo_keeps_the_left_channel(tmp_path):
    """A stereo file read as mono would halve every reported frequency."""
    path = tmp_path / "stereo.wav"
    write_wav(path, make_tone(FREQ, SECONDS, RATE, 0.5, stereo=True), channels=2)
    data, rate = read_wav(str(path))
    assert rate == RATE
    assert len(data) == int(RATE * SECONDS)
    assert dominant_peak(data, rate) == pytest.approx(FREQ, abs=0.5)


def test_read_wav_multichannel_keeps_the_first_channel(tmp_path):
    """Four channels: the same interleaving rule, not a special case for two."""
    path = tmp_path / "quad.wav"
    tone = make_tone(FREQ, SECONDS, RATE, 0.5)
    interleaved = np.zeros(tone.size * 4, dtype=np.int16)
    interleaved[0::4] = tone
    write_wav(path, interleaved, channels=4)
    data, rate = read_wav(str(path))
    assert len(data) == int(RATE * SECONDS)
    assert dominant_peak(data, rate) == pytest.approx(FREQ, abs=0.5)


def test_make_tone_stereo_is_interleaved_with_a_silent_right_channel():
    samples = make_tone(FREQ, SECONDS, RATE, 0.5, stereo=True)
    assert samples.size == 2 * int(RATE * SECONDS)
    assert np.all(samples[1::2] == 0)
    assert dominant_peak(samples[0::2].astype(np.float64) / 32768.0,
                         RATE) == pytest.approx(FREQ, abs=0.5)


def test_read_wav_rejects_non_16_bit(tmp_path):
    path = tmp_path / "eight_bit.wav"
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(1)
        w.setframerate(RATE)
        w.writeframes(b"\x80" * 100)
    with pytest.raises(SystemExit):
        read_wav(str(path))
