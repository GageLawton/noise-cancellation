"""Every script in tools/ must be linked from README.md.

New scripts are supposed to get a line in the README's "Other tools in
tools/" list, so people doing a bench check can find them. This test fails
when one is missing, naming the script.
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOOLS = ROOT / "tools"
README = ROOT / "README.md"


def tool_scripts():
    """Return the names of the scripts in tools/, without tests."""
    return sorted(
        path.name
        for path in TOOLS.glob("*.py")
        if not path.name.startswith("test_") and path.name != "conftest.py"
    )


def test_tools_folder_has_scripts():
    assert tool_scripts(), "found no scripts in tools/"


def test_every_tool_is_linked_from_the_readme():
    readme = README.read_text(encoding="utf-8")
    missing = [name for name in tool_scripts() if f"](tools/{name})" not in readme]
    assert not missing, (
        "README.md has no link to "
        + ", ".join(f"tools/{name}" for name in missing)
        + '; add a line for each to the "Other tools in tools/" list'
    )
