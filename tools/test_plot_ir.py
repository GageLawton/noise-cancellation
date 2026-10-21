"""plot_ir.py reads the impulse response CSV that ir_from_log.py writes."""

import pytest
from plot_ir import delays_ms, load_ir_csv, title_for


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
