# ANC — Tonal Active Noise Cancellation

[![CI](https://github.com/GageLawton/noise-cancellation/actions/workflows/ci.yml/badge.svg)](https://github.com/GageLawton/noise-cancellation/actions/workflows/ci.yml)

Coordinated multi-node active noise cancellation on ESP32-S3. Narrowband
FxLMS cancellation of tonal noise (HVAC hum, transformer buzz), built to
scale to synchronized multi-node hardware.

Full design rationale: [`docs/design.md`](docs/design.md).

Hardware setup: [`docs/hardware.md`](docs/hardware.md).

## Status

The device boots, loads its JSON config from flash, runs the state machine
and reports status at 1 Hz. The building blocks for M2 to M6 (I2S driver,
test tone, calibration, FxLMS canceller and frequency alignment) exist and
are tested on a laptop; wiring them into the running firmware is in
progress. The [roadmap (#138)](https://github.com/GageLawton/noise-cancellation/issues/138)
lists what's left, issue by issue.

| M | Goal | Status |
|---|---|---|
| M0 | Both build targets compile | done |
| M1 | Boot path, config, tasks, telemetry | done |
| M2 | Audio plumbing end-to-end | in progress |
| M3 | Output path proven (fixed sine) | building blocks done |
| M4 | Calibration (probe → SecondaryPath) | building blocks done |
| M5 | Cancellation (FxLMS) | building blocks done |
| M6 | Alignment (reconstruct, PLL, drift) | building blocks done |
| M7 | Multi-node prep (sync line) | |

## Get involved

- Start with a [good first issue](https://github.com/GageLawton/noise-cancellation/labels/good%20first%20issue).
- Most tasks need only a laptop: browse [no hardware needed](https://github.com/GageLawton/noise-cancellation/labels/no%20hardware%20needed) issues.
- Read [CONTRIBUTING.md](CONTRIBUTING.md) for setup, testing, and pull request instructions.
- Want to build the board? Follow the [hardware guide](docs/hardware.md).
- Comment on an issue to claim it before starting work.

## Setting up your computer

You need Git, CMake 3.20+, a C++17 compiler (GCC or Clang), and
clang-format. No ESP32 is needed for anything in this section.

**Windows** (PowerShell):

```powershell
winget install Git.Git
winget install Kitware.CMake
winget install BrechtSanders.WinLibs.POSIX.UCRT.LLVM
```

The last package is a GCC + LLVM toolchain that includes `g++` and
`clang-format`. Close and reopen PowerShell afterwards so the new tools
are on your `PATH`.

**macOS** (Terminal, with [Homebrew](https://brew.sh)):

```bash
xcode-select --install          # Apple's compiler; skip if already installed
brew install cmake clang-format
```

**Linux** (Debian/Ubuntu):

```bash
sudo apt install git cmake g++ clang-format
```

Check everything is found (these work in any terminal):

```bash
git --version
cmake --version
clang-format --version
```

## Host tests

No hardware needed. The DSP core, alignment, and calibration components
have no ESP-IDF dependency by design, so they build and run on any
machine — which is where most of the real debugging happens.

macOS and Linux:

```bash
cd test
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure
```

Windows (PowerShell). The `-G "MinGW Makefiles"` tells CMake to use the
GCC toolchain installed above instead of Visual Studio:

```powershell
cd test
cmake -B build -S . -G "MinGW Makefiles"
cmake --build build
ctest --test-dir build --output-on-failure
```

Unity is fetched automatically on first configure, so the first run needs
network access. If you change generators, delete `test/build` first.

## Coverage

```bash
pip install gcovr
./scripts/coverage.sh          # add --open to view the HTML report
```

The script is bash, so it runs on Linux, macOS, or
[WSL](https://learn.microsoft.com/windows/wsl/install) on Windows. You
don't have to run it locally: the Coverage job in CI runs it on every PR.

Current baseline is ~91% lines, 100% functions. CI fails below 85% —
deliberately below the current number so it catches a real regression
rather than failing on noise. Raise the threshold as the DSP core fills
in and coverage becomes more meaningful.

A caveat on reading the numbers: gcov accounts for each template
instantiation separately, so header-only templates like
`SpscRingBuffer` report a few lines uncovered even when another
instantiation exercises them heavily. 100% is not a realistic target
there.

## Formatting

Run from the repo root before you commit.

macOS and Linux:

```bash
clang-format -i $(find components main test -path test/build -prune -o \( -name '*.cpp' -o -name '*.hpp' \) -print)
```

Windows (PowerShell):

```powershell
clang-format -i (Get-ChildItem components,main,test -Recurse -Include *.cpp,*.hpp | Where-Object FullName -NotMatch '\\build\\').FullName
```

CI checks this and fails on any diff. Config is in `.clang-format`. CI
uses clang-format 18; a much newer version can occasionally format a line
differently, and the Format check log shows the exact change it wants.

## CI

Five jobs run on every push and PR ([`.github/workflows/ci.yml`](.github/workflows/ci.yml)):

| Job | What it catches |
|---|---|
| Host tests | Logic regressions in portable code — fast, no hardware |
| Python tools | Breakage in the offline analysis scripts under `tools/` |
| Coverage | Untested new code, via an 85% line-coverage gate |
| Firmware build | ESP-IDF component/driver mistakes host tests can't see |
| Format check | Style drift |

The firmware build is the only job that compiles against real ESP-IDF —
host tests never touch it, so component dependency errors surface only
here.

## Device build

Requires [ESP-IDF v5.3](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/get-started/),
the same version CI builds with. The Espressif installer for Windows, or
the macOS/Linux install steps on that page, set it up.

macOS and Linux:

```bash
. $IDF_PATH/export.sh          # once per terminal
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Windows: open the **ESP-IDF 5.3 PowerShell** shortcut the installer
created (it has the environment already set up), `cd` to the repo, then:

```powershell
idf.py set-target esp32s3
idf.py build
idf.py -p COM3 flash monitor
```

Replace the port with your board's. On Windows, Device Manager lists it
under **Ports (COM & LPT)**. On macOS it looks like `/dev/cu.usbserial-*`
or `/dev/cu.usbmodem*` (run `ls /dev/cu.*`). On Linux it is usually
`/dev/ttyUSB0` or `/dev/ttyACM0`. Press `Ctrl+]` to leave the monitor.

On first flash the config partition is blank; the firmware formats it and
writes default settings automatically. Expected serial output:

```
I (xxx) runtime: ANC node starting
I (xxx) config: littlefs mounted at /cfg
I (xxx) config: no config file, writing defaults
I (xxx) runtime: node id xxxxxxxxxxxx
I (xxx) runtime: name=anc-node taps=128 mu=0.100 block=32 tones=1
I (xxx) runtime: BOOT -> INIT_HAL
I (xxx) telemetry: state=RUN conv=0 resid=0.000000 freq=60.00 lock=0 clip=0
```

### Flashing tips and troubleshooting

**Use the port labelled COM (or UART), not USB.** The ESP32-S3 DevKitC
has two USB-C ports. The COM/UART one goes through a USB-serial chip:
esptool can reset the board into flashing mode through it automatically,
and the firmware's log output comes out there. The USB port is the chip's
own USB. It shows up with VID `303A`, it won't show these logs, and
whatever firmware is running can stop esptool reaching the bootloader
through it.

**Find the right port.** This works in both shells and lists every
serial port with a description:

```bash
python -m serial.tools.list_ports -v
```

The board's COM/UART port mentions `CP210x` or `CH343`. If no such port
appears, install the matching driver (Silicon Labs CP210x or WCH CH343,
whichever chip is printed next to the port). To double-check, unplug the
board and the port should disappear from the list.

**`Failed to connect ... No serial data received`.** The port opened but
the chip didn't answer. In order:

1. Check you're on the COM/UART port (above).
2. Force the ROM bootloader: hold **BOOT**, tap **RST**, release
   **BOOT**, then flash again. Adding `-b 115200` slows the transfer down,
   which helps with marginal cables.
3. Take the board out of the breadboard. On many breadboards the
   DevKitC's two pin rows end up shorted together.

**Windows build environment.** Run `idf.py` from the ESP-IDF 5.3 CMD or
PowerShell shortcut, not from Git Bash or MSYS2, where `idf.py` isn't
found. Keep the clone outside OneDrive (for example `C:\esp\noise-cancellation`),
because OneDrive locks files while the build writes them, and keep the
path short and free of spaces.

## Analysis tools

[`tools/analyze.py`](tools/analyze.py) measures how much the canceller
reduces a tone. It compares two recordings from the external measurement
mic, one with cancellation off (baseline) and one with it running, and
reports the drop in dB around the given frequency. It needs Python 3 and
numpy.

macOS and Linux:

```bash
pip install -r tools/requirements.txt
python3 tools/analyze.py baseline.wav cancelled.wav --freq 60
```

Windows (PowerShell):

```powershell
pip install -r tools/requirements.txt
python tools/analyze.py baseline.wav cancelled.wav --freq 60
```

Run `python3 tools/analyze.py --help` (or `python` on Windows) to see all
options.

No recordings yet? [`tools/make_tone.py`](tools/make_tone.py) writes a
16-bit WAV with a tone of a known frequency, which is enough to exercise
the analysis on a laptop:

```bash
python3 tools/make_tone.py tone.wav --freq 120
python3 tools/analyze.py tone.wav          # target frequency: 120.00 Hz
```

The tools have their own tests, run by the **Python tools** CI job:

```bash
python -m pytest tools
```

## Configuration

Runtime settings live in `/cfg/node.json` on the device's LittleFS
partition. Every tunable value is there — changing one does **not**
require recompiling.

```json
{
  "identity": { "node_name": "anc-node" },
  "dsp": {
    "secondary_path_taps": 128,
    "normalized_step_size": 0.1,
    "dma_block_samples": 32
  },
  "calibration": {
    "probe_duration_ms": 300,
    "correlation_threshold": 0.3,
    "correlation_window_ms": 1000,
    "backstop_interval_ms": 600000,
    "cooldown_ms": 30000
  },
  "tracking": {
    "initial_freq_hz": [60.0],
    "max_drift_rate_hz_per_sec": 2.0,
    "watch_correlation_threshold": 0.15
  },
  "output": {
    "ceiling": 0.8,
    "test_tone_hz": 440.0,
    "test_tone_amplitude": 0.1
  }
}
```

`initial_freq_hz` is a starting hint for the frequency tracker, not a
fixed target — the PLL walks to whatever tone actually dominates.

[`docs/config.md`](docs/config.md) explains every setting, its default and
its allowed values.

Anything the config file omits falls back to its compiled-in default, and
a config that fails validation is rejected in favour of defaults, so the
device always boots into a working state.

## Layout

```
components/
  anc_hal/       I2S, GPIO sync. Only layer touching ESP-IDF drivers
  dsp_core/      FirFilter, FxlmsCanceller, RefGenerator, and the block
                 processors FilterTask runs (silence, tone, canceller)
  alignment/     Reconstructor, Pll, DriftDetector
  calibration/   probe generation, impulse response → FIR estimate
  config/        JSON load/save, NodeConfig
  node_runtime/  state machine, task setup
  coordinator/   multi-node stub
  transport/     wired GPIO sync shim
main/            app_main.cpp
test/            host build + Unity tests
  fakes/         fakes and the simulated room for host tests
docs/            design, config, hardware and glossary pages
tools/           offline analysis scripts
scripts/         dev helpers (coverage)
```
