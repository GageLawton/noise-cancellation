# Bench results

Measurements taken on real hardware, kept in one place so they're easy to
find after the bench-check issues close. Each bench check has its own
section below with a table to fill in.

Add one row per run, and fill in every column:

- **Date** the run was made (YYYY-MM-DD).
- **Commit** tested: the short hash from `git rev-parse --short HEAD`.
- **Hardware** used, if it differs from the [Hardware](#hardware) section
  (for example a different speaker or mic position).

Leave a cell as `-` if it doesn't apply, and put anything unusual in
**Notes**. Paste the full serial output or recording details in the
issue, and link it from **Notes**.

## M2 audio loop (#92)

Bench check [#92](https://github.com/GageLawton/noise-cancellation/issues/92):
run the M2 audio loop for 10 minutes without dropouts.

| Date | Commit | Hardware | Run length | blocks/s | filter/s | align/s | Underruns | Drops | Mic dBFS (quiet) | Mic dBFS (clap) | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| | | | | | | | | | | | |

## M3 test tone (#100)

Bench check [#100](https://github.com/GageLawton/noise-cancellation/issues/100):
measure the test tone with the measurement mic.

| Date | Commit | Hardware | Target Hz | Measured Hz | Audible (yes/no) | Notes |
|---|---|---|---|---|---|---|
| | | | | | | |

## M4 speaker-to-mic delay (#110)

Bench check [#110](https://github.com/GageLawton/noise-cancellation/issues/110):
confirm 128 taps covers the measured speaker-to-mic delay.
[`tools/ir_from_log.py`](../tools/ir_from_log.py) prints the peak index,
value and delay from a serial log.

| Date | Commit | Hardware | Mic distance | Peak index | Peak value | Delay (ms) | Settled before tap 128 (yes/no) | Notes |
|---|---|---|---|---|---|---|---|---|
| | | | | | | | | |

## M5 cancellation (#122)

Bench check [#122](https://github.com/GageLawton/noise-cancellation/issues/122):
measure the first real cancellation in dB.
[`tools/analyze.py`](../tools/analyze.py) gives the depth from a baseline
and a cancelled recording.

| Date | Commit | Hardware | Frequency (Hz) | Depth (dB) | Time to converge | Notes |
|---|---|---|---|---|---|---|
| | | | | | | |

## Hardware

The setup these results were measured on. The parts and wiring are in
[`hardware.md`](hardware.md).

| Part | Model |
|---|---|
| Board | ESP32-S3-DevKitC-1 |
| Microphone | INMP441 I2S microphone breakout |
| Amplifier | MAX98357A I2S amplifier breakout |
| Speaker | Small 4 to 8 Ω speaker, exact model: ______ |
| Measurement mic | ______ |
