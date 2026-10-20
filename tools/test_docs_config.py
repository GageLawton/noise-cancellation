"""Every config key the firmware accepts must have a row in docs/config.md.

The keys come from components/config/include/anc/config_keys.hpp, which
maps "section.key" names to NodeConfig fields for the serial `set` and
`get` commands.
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
KEYS_HEADER = ROOT / "components" / "config" / "include" / "anc" / "config_keys.hpp"
CONFIG_DOC = ROOT / "docs" / "config.md"

KEY_PATTERN = re.compile(r'\{"([a-z_]+)\.([a-z_]+)"')


def config_keys():
    """Returns (section, key) pairs, in the order the header lists them."""
    return KEY_PATTERN.findall(KEYS_HEADER.read_text(encoding="utf-8"))


def section_text(doc, section):
    """Returns the text between `## section` and the next `## ` heading."""
    match = re.search(
        rf"^## {re.escape(section)}\s*$(.*?)(?=^## |\Z)", doc, re.MULTILINE | re.DOTALL
    )
    return match.group(1) if match else ""


def undocumented(keys, doc):
    """Returns "section.key" for each key with no table row in its section."""
    missing = []
    for section, key in keys:
        row = re.compile(rf"^\| `{re.escape(key)}` \|", re.MULTILINE)
        if not row.search(section_text(doc, section)):
            missing.append(f"{section}.{key}")
    return missing


def test_header_lists_enough_keys():
    # Guards against the regex silently matching nothing.
    assert len(config_keys()) >= 10


def test_every_key_is_documented():
    missing = undocumented(config_keys(), CONFIG_DOC.read_text(encoding="utf-8"))
    assert not missing, f"undocumented: {missing}"


def test_missing_row_is_reported():
    doc = "## output\n\n| `ceiling` | `0.8` |\n\n## dsp\n\n| `other` | 1 |\n"
    assert undocumented(
        [("output", "ceiling"), ("dsp", "secondary_path_taps")], doc
    ) == ["dsp.secondary_path_taps"]


def test_row_under_the_wrong_section_is_reported():
    doc = "## output\n\n## dsp\n\n| `ceiling` | `0.8` |\n"
    assert undocumented([("output", "ceiling")], doc) == ["output.ceiling"]
