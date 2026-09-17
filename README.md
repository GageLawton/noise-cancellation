# ANC — Tonal Active Noise Cancellation

[![CI](https://github.com/GageLawton/noise-cancellation/actions/workflows/ci.yml/badge.svg)](https://github.com/GageLawton/noise-cancellation/actions/workflows/ci.yml)

Coordinated multi-node active noise cancellation on ESP32-S3. Narrowband
FxLMS cancellation of tonal noise (HVAC hum, transformer buzz), built to
scale to synchronized multi-node hardware.

Full design rationale: [`docs/design.md`](docs/design.md).

## Status

**M1 complete.** The device boots, loads its JSON config from flash, runs
the state machine, and reports status at 1Hz. No audio or DSP yet — see
the milestone table in the design doc (section 10.4).

| M | Goal | Status |
|---|---|---|
| M0 | Both build targets compile | done |
| M1 | Boot path, config, tasks, telemetry | done |
| M2 | Audio plumbing end-to-end | next |
| M3 | Output path proven (fixed sine) | |
| M4 | Calibration (probe → SecondaryPath) | |
| M5 | Cancellation (FxLMS) | |
| M6 | Alignment (reconstruct, PLL, drift) | |
| M7 | Multi-node prep (sync line) | |

## Host tests

No hardware needed. The DSP core, alignment, and calibration components
have no ESP-IDF dependency by design, so they build and run on any
machine — which is where most of the real debugging happens.

```bash
cd test
cmake -B build -S .
cmake --build build
cd build && ctest --output-on-failure
```

Unity is fetched automatically on first configure. Requires CMake 3.16+,
a C++17 compiler, and network access for the initial fetch.

## Coverage

```bash
pip install gcovr
./scripts/coverage.sh          # add --open to view the HTML report
```

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

```bash
clang-format -i $(find components main test -name '*.cpp' -o -name '*.hpp')
```

CI checks this and fails on any diff. Config is in `.clang-format`.

## CI

Four jobs run on every push and PR ([`.github/workflows/ci.yml`](.github/workflows/ci.yml)):

| Job | What it catches |
|---|---|
| Host tests | Logic regressions in portable code — fast, no hardware |
| Coverage | Untested new code, via an 85% line-coverage gate |
| Firmware build | ESP-IDF component/driver mistakes host tests can't see |
| Format check | Style drift |

The firmware build is the only job that compiles against real ESP-IDF —
host tests never touch it, so component dependency errors surface only
here.

## Device build

Requires [ESP-IDF v5.0+](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/).

```bash
. $IDF_PATH/export.sh          # once per shell
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

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
  }
}
```

`initial_freq_hz` is a starting hint for the frequency tracker, not a
fixed target — the PLL walks to whatever tone actually dominates.

Anything the config file omits falls back to its compiled-in default, and
a config that fails validation is rejected in favour of defaults, so the
device always boots into a working state.

## Layout

```
components/
  hal/           I2S, GPIO sync. Only layer touching ESP-IDF drivers
  dsp_core/      FirFilter, FxlmsCanceller, RefGenerator
  alignment/     Reconstructor, Pll, DriftDetector
  calibration/   probe generation, impulse response → FIR estimate
  config/        JSON load/save, NodeConfig
  node_runtime/  state machine, task setup
  coordinator/   multi-node stub
  transport/     wired GPIO sync shim
main/            app_main.cpp
test/            host build + Unity tests
tools/           offline analysis scripts
scripts/         dev helpers (coverage)
```
