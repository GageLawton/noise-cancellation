"""Unit tests for fxlms_sim.py."""

from fxlms_sim import diverged, reduction_db, simulate


def test_small_step_cancels_the_hum():
    mic = simulate(0.005)

    assert not diverged(mic)
    assert reduction_db(mic) > 20.0


def test_large_step_diverges_through_the_loop_delay():
    # The firmware's own test (test_fxlms_canceller.cpp) shows 0.1, the old
    # default, going unstable through a 180-sample path.
    assert diverged(simulate(0.1))
