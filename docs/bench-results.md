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
confirm that `secondary_path_taps` covers the measured speaker-to-mic delay.
[`tools/ir_from_log.py`](../tools/ir_from_log.py) prints the peak index,
value and delay from a serial log, and
[`tools/plot_ir.py`](../tools/plot_ir.py) draws the response.

| Date | Commit | Hardware | Mic distance | Taps | Peak index | Peak value | Delay (ms) | Settled before the last tap (yes/no) | Notes |
|---|---|---|---|---|---|---|---|---|---|
| | | | | | | | | | |

## M5 cancellation (#122)

Bench check [#122](https://github.com/GageLawton/noise-cancellation/issues/122):
measure the first real cancellation in dB.
[`tools/analyze.py`](../tools/analyze.py) gives the depth from a baseline
and a cancelled recording.

| Date | Commit | Hardware | Frequency (Hz) | Step size | Depth (dB) | Time to converge | Notes |
|---|---|---|---|---|---|---|---|
| | | | | | | | |

## First boot (#253)

Bench check [#253](https://github.com/GageLawton/noise-cancellation/issues/253):
first boot of the full firmware, from BOOT to RUN.

| Date | Commit | Reached RUN (yes/no) | Peak tap | Delay ms | Underruns after 2 min | Notes |
|---|---|---|---|---|---|---|
| | | | | | | |

## Hum level and lock (#264)

Bench check [#264](https://github.com/GageLawton/noise-cancellation/issues/264):
how loud is the hum the frequency tracker sees?

| Date | Commit | Hum source | hum dBFS | Locked at default (yes/no) | Lowest min_lock_amplitude that locked | Notes |
|---|---|---|---|---|---|---|
| | | | | | | |

## Stack headroom (#265)

Bench check [#265](https://github.com/GageLawton/noise-cancellation/issues/265):
stack and timing headroom over 10 minutes. Each task column is the smallest
free stack seen, in bytes.

| Date | Commit | audio_io | filter | alignment | config | telemetry | Notes |
|---|---|---|---|---|---|---|---|
| | | | | | | | |

## Sync loopback (#267)

Bench check [#267](https://github.com/GageLawton/noise-cancellation/issues/267):
sync tick loopback with a jumper wire.

| Date | Commit | Received with jumper | Received without | Worst round trip µs | Notes |
|---|---|---|---|---|---|
| | | | | | |

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
