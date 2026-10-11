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
