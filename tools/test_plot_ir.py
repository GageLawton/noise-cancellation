"""plot_ir.py reads the impulse response CSV that ir_from_log.py writes."""

import pytest
from plot_ir import delays_ms, load_ir_csv, taps_within, title_for


def test_load_ir_csv_reads_rows_back(tmp_path):
    path = tmp_path / "ir.csv"
    path.write_text("index,value\n0,0.001\n1,0.431\n2,-0.0125\n", encoding="utf-8")

    pairs = load_ir_csv(path)

    assert [index for index, _ in pairs] == [0, 1, 2]
    assert [value for _, value in pairs] == pytest.approx([0.001, 0.431, -0.0125])


def test_delays_ms_uses_the_sample_rate():
    pairs = [(0, 0.0), (8, 0.5)]
    assert delays_ms(pairs, 8000) == pytest.approx([0.0, 1.0])
    assert delays_ms(pairs, 16000) == pytest.approx([0.0, 0.5])


def test_title_for_one_response():
    pairs = [(0, 0.1), (8, -0.5), (9, 0.2)]
    assert title_for(pairs, None, 8000) == "Impulse response: peak -0.500 at 1.000 ms"


def test_title_for_two_responses_reports_how_far_the_peak_moved():
    before = [(i, 1.0 if i == 8 else 0.0) for i in range(32)]
    after = [(i, 1.0 if i == 16 else 0.0) for i in range(32)]
    assert title_for(before, after, 8000) == (
        "Impulse responses: peak moved by 1.000 ms"
    )


def test_taps_within_keeps_taps_up_to_max_ms():
    pairs = [(i, 0.1 * i) for i in range(256)]
    kept = taps_within(pairs, 8000, 1.0)
    assert [index for index, _ in kept] == list(range(9))
    assert kept == pairs[:9]


def test_taps_within_a_large_max_ms_keeps_everything():
    pairs = [(i, 0.1 * i) for i in range(256)]
    assert taps_within(pairs, 8000, 1000.0) == pairs
