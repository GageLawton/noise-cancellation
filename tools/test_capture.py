"""Unit tests for capture.py."""

from capture import stamp


def test_stamp_prefixes_the_elapsed_seconds():
    assert stamp(b"I (1234) anc: ready\n", 12.3456) == "12.346 I (1234) anc: ready"


def test_stamp_strips_a_windows_line_ending():
    assert stamp(b"hello\r\n", 0.0) == "0.000 hello"


def test_stamp_replaces_a_bad_byte():
    assert stamp(b"bad \xff byte\n", 1.5) == "1.500 bad � byte"
