# Contributing

Thanks for helping out! This guide should take about two minutes to read.
It works on Windows, macOS, and Linux. If you haven't installed the tools
yet, follow [Setting up your computer](README.md#setting-up-your-computer)
in the README first.

## Finding something to work on

Start with issues labelled
[`good first issue`](https://github.com/GageLawton/noise-cancellation/labels/good%20first%20issue)
and
[`no hardware needed`](https://github.com/GageLawton/noise-cancellation/labels/no%20hardware%20needed).
Leave a comment on the issue saying you're taking it, so two people don't
do the same work. Questions go in a comment on the issue too.

## What the labels mean

Every issue has a few labels. They tell you how big it is, whether you
need the board, and what part of the code it touches.

| Label | What it means for you |
|---|---|
| `good first issue` | Small and well described, a good place to start. |
| `help wanted` | Open for anyone to pick up; extra hands are welcome. |
| `stretch` | A bigger task (30 to 60 minutes) for when you want to dig in. |
| `no hardware needed` | You can do and test it on a laptop, no ESP32 or audio parts. |
| `hardware needed` | You need the ESP32-S3 board with the mic and amp wired up. |
| `bench bring-up` | Running and measuring the full firmware on a real board. |
| `roadmap` | A tracking issue for the project plan, not a task to pick up. |
| `M2` to `M7` | Which milestone the issue belongs to, from `M2: audio plumbing` to `M7: multi-node`; see the [build order](docs/design.md#104-build-order-skeleton-first). |
| `c++` | Touches the C++ firmware or the portable library code. |
| `python` | Touches the Python tools in `tools/`. |
| `dsp` | Signal-processing math; portable and testable on a laptop. |
| `tests` | Adds or improves the host unit tests. |
| `documentation` | Changes the docs, not the code. |
| `ci` | GitHub Actions and other repository automation. |

The full list is on the
[Labels page](https://github.com/GageLawton/noise-cancellation/labels).

## Running the host tests

No ESP32 needed. From the repo root:

macOS and Linux:

```bash
cd test
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure
```

Windows (PowerShell):

```powershell
cd test
cmake -B build -S . -G "MinGW Makefiles"
cmake --build build
ctest --test-dir build --output-on-failure
```

Or run `./scripts/test.sh` (macOS and Linux) or `.\scripts\test.ps1` (Windows PowerShell), which does all three steps.

## Formatting

From the repo root, before you commit:

macOS and Linux:

```bash
./scripts/format.sh
# Use ./scripts/format.sh --check to verify formatting without changing files.
```

Windows (PowerShell):

```powershell
clang-format -i (Get-ChildItem components,main,test -Recurse -Include *.cpp,*.hpp | Where-Object FullName -NotMatch '\\build\\').FullName
```

CI rejects code that isn't formatted.

## Opening a pull request

1. Fork the repo and create a branch for your change.
2. Keep the PR small: one issue per PR is ideal.
3. In the description, link the issue (`Fixes #123`) and say how you
   tested it.
4. Make sure CI is green. If a check fails, its log says why.

## Hardware

The firmware targets an ESP32-S3 with an I2S microphone and an I2S
amplifier (see [`docs/design.md`](docs/design.md)). You don't need any of
it for most issues: anything labelled `no hardware needed` runs entirely
on your computer.

## License

By contributing, you agree that your contributions are licensed under the
[MIT License](LICENSE).
