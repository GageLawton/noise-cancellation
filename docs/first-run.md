# First run: what the serial log should look like

This page tells you what a healthy board prints the first time you flash
the full firmware, so you can tell at a glance whether something is wrong.
It was written from the code in
[`node_runtime.cpp`](../components/node_runtime/node_runtime.cpp), so if a
message here doesn't match what you see, the code wins (and please open an
issue).

## Before you flash

- Wire the board as described in [hardware.md](hardware.md).
- Build and flash as described in the README's
  [Device build](../README.md#device-build) section.

If the board has run older firmware, erase it once first
(`idf.py -p PORT erase-flash`) so it doesn't keep old saved settings.

## What you should see

The node moves through these states, and logs a line like
`runtime: BOOT -> INIT_HAL` each time it changes:

```
BOOT -> INIT_HAL -> CALIBRATE_SECONDARY_PATH -> RUN
```

| State | What it does | What it logs |
|---|---|---|
| `BOOT` | Reads the node id from the chip, mounts the flash and loads `/cfg/node.json` (or writes the defaults on first flash). | `ANC node starting`, `node id ...`, then `name=... taps=... mu=... block=... tones=...` with the settings it loaded. |
| `INIT_HAL` | Starts I2S for the mic and the amp, then starts the tasks. | `tasks spawned; type help for serial commands`. |
| `CALIBRATE_SECONDARY_PATH` | Plays a short burst of white noise and records the mic, to measure the path from the speaker to the mic. You will hear a hiss for about a second. | `calibration: playing ... ms probe`, `calibration: peak at tap ..., value ..., delay ... ms`, then one `ir,<tap>,<value>` line per tap. |
| `RUN` | Plays the anti-noise that cancels the hum, and tracks the hum's frequency as it drifts. | `cancelling 1 tone(s) from 60.0 Hz, mu 0.0050`. |

Two more states show up only when something needs attention:

- `RECHECK_SECONDARY_PATH` runs the calibration again. You'll see
  `rechecking the speaker-to-mic path:` followed by the reason: `requested`
  (you typed `recal`), `the model no longer matches`, or
  `backstop interval reached` (every 10 minutes by default).
- `FAULT` mutes the output at once (`fault -- output muted`) and then waits
  in `SAFE_MUTE` before trying again. The line just before it says why, for
  example `divergence detected` or `I2S init failed, check the mic and amp
  wiring`.

## Reading the telemetry line

Once the tasks start, a line like this appears once a second:

```
I (12345) telemetry: state=RUN conv=1 resid=-48.2dBFS freq=60.02 lock=1 corr=0.05 low=0 recal=0 clip=0 blocks=2500 filter=2500 under=0 align=80000 drop=0 mic=-38.5dBFS hum=-52.0dBFS wnorm=0.412 faults=0
```

| Field | What it means | Healthy value |
|---|---|---|
| `state` | The node's current state (see above). | `RUN` after a few seconds. |
| `conv` | 1 when the canceller's residual has stayed low for half a second. | 1 within a few seconds of reaching `RUN`. |
| `resid` | How loud the hum still is after cancelling, in dBFS. Only updated in `RUN`. | Lower is better; it should fall after `RUN` starts. |
| `freq` | The hum frequency the tracker is following, in Hz. Shows the configured starting hint until tracking takes over. | Close to your mains frequency (60 or 50 Hz). |
| `lock` | 1 when the tracker has locked on to the hum. | 1. It stays 0 if the hum is quieter than `tracking.min_lock_amplitude` or there is no hum. |
| `corr` | How much of what the mic hears still looks like our own speaker output, from 0 to 1. | Low (well under 0.15). |
| `low` | 1 when `corr` is above the watch threshold, so the frequency estimate isn't trusted. | 0. |
| `recal` | 1 when the speaker-to-mic model looks stale and a recalibration is wanted. | 0 most of the time. |
| `clip` | Total speaker samples the limiter had to clip. | 0, or only grows during loud events. |
| `blocks` | Audio blocks sent to the speaker since boot (32 samples each). | Grows by about 250 every second. |
| `filter` | Blocks the filter task processed since boot. | Grows at the same rate as `blocks`. |
| `under` | Speaker samples that were missing and played as silence. | Stays at 0. |
| `align` | Sample pairs the alignment task received since boot. | Grows by about 8000 every second. |
| `drop` | Sample pairs the alignment task missed because it fell behind. | Stays at 0. |
| `mic` | Mic level in dBFS. Rises at once on a loud sound and falls back slowly. | Roughly -60 to -30 in a quiet room; a clap should jump it up. |
| `hum` | Level of just the hum at the tracked frequency (`freq`), in dBFS, measured on the mic over each 100 ms. Shows -120.0 until the first measurement. | Falls after `RUN` starts as the canceller takes the hum out; well below `mic`. |
| `wnorm` | Size of the canceller's weights (the square root of the sum of their squares). Only updated in `RUN`. | Settles at a steady value. If it keeps growing, the canceller is diverging. |
| `faults` | How many times the node has entered `FAULT` since boot. | 0. |

Every 10 seconds two more lines appear:

```
I (20345) telemetry: stack free: audio_io=1820 filter=2440 alignment=3100 config=2900 telemetry=1650
I (20345) telemetry: coordinator: 1 node(s)
```

`stack free:` is the least stack each task has ever had free, in bytes
(its high-water mark); 0 means that task never started. A task under about
512 bytes free is a warning sign: it is close to running out, which crashes
the board, so save the log and open an issue. `coordinator:` is how many
nodes the coordinator has heard from; on one board it should always say 1.

To see the current state without waiting for the next telemetry line, type
`status` in the monitor. It prints the state, the tracked frequency, the
lock, the residual and the mic level straight away:

```
state=RUN freq=60.02 lock=1 resid=-48.2dBFS mic=-38.5dBFS
```

## Calibration warnings

`doCalibrate()` can log these warnings. Calibration still finishes, but the
canceller is likely to work badly until you fix the cause.

| Warning | What it means | What to try |
|---|---|---|
| `probe_duration_ms ... is longer than the recorder holds; using ... ms` | `calibration.probe_duration_ms` is larger than the recording buffer, so the probe was shortened. | Nothing urgent. Lower the setting to stop the warning. |
| `calibration: no clear peak -- is the speaker playing and the mic connected?` | The mic didn't clearly hear the probe. | Check the amp and mic wiring, turn the volume up, and move the mic closer to the speaker. Make sure you heard the hiss. |
| `calibration: peak at tap ... leaves under 64 taps for the speaker's ring; raise dsp.secondary_path_taps` | The delay from speaker to mic is so long that the model can't hold the speaker's ring after it. | Move the mic closer to the speaker, or `set dsp.secondary_path_taps` to a larger value and `save`. |

Errors (lines starting with `E`) such as `calibration: probe didn't finish`
or `calibration: FilterTask not responding` send the node to `FAULT`. Save
the log and open an issue.

## Saving a log

Keep the monitor output when you report a problem or fill in a bench check.

bash:

```bash
idf.py -p PORT monitor | tee run.log
```

PowerShell:

```powershell
idf.py -p PORT monitor | Tee-Object run.log
```

Turn the telemetry lines into a table you can open in a spreadsheet with
[`tools/telemetry_to_csv.py`](../tools/telemetry_to_csv.py):

```bash
python tools/telemetry_to_csv.py run.log --out telemetry.csv
```
