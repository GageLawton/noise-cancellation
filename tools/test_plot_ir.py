"""plot_ir.py reads the impulse response CSV that ir_from_log.py writes."""

import pytest
from plot_ir import load_ir_csv


def test_load_ir_csv_reads_rows_back(tmp_path):
    path = tmp_path / "ir.csv"
    path.write_text("index,value\n0,0.001\n1,0.431\n2,-0.0125\n", encoding="utf-8")

    pairs = load_ir_csv(path)

    assert [index for index, _ in pairs] == [0, 1, 2]
    assert [value for _, value in pairs] == pytest.approx([0.001, 0.431, -0.0125])
