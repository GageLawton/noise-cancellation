# Configuration reference

The firmware reads `/cfg/node.json` from its LittleFS flash filesystem at boot.
The file contains four JSON sections. Omitted settings keep their compiled-in
defaults. A missing, unparseable, or invalid configuration is replaced with the
defaults in memory; writing those defaults back to flash is best-effort.

This page follows [the JSON reader](../components/config/config_store.cpp),
[defaults and validation](../components/config/node_config.cpp), and
[field definitions](../components/config/include/anc/node_config.hpp).
Audio, calibration, and tracking descriptions explain the intended role of
each setting; those operations are not all implemented in the current firmware.

## identity

| Key | Default | Allowed values | What it does |
|---|---|---|---|
| `node_name` | `"anc-node"` | Nonempty string; at most 31 bytes are stored. Longer names are truncated by the reader. | Human-readable label for logs. It does not determine the MAC-derived machine identity. |

## dsp

| Key | Default | Allowed values | What it does |
|---|---|---|---|
| `secondary_path_taps` | `128` | Whole number from 8 through 1024, inclusive. | Number of weights in the speaker-to-microphone model. At 8 kHz, 128 taps span 16 ms of sampled history. |
| `normalized_step_size` | `0.1` | Greater than 0 and less than 2; endpoints excluded. | Learning-rate setting for the planned normalized adaptive filter. Larger adjustments are faster but can be less stable. |
| `dma_block_samples` | `32` | Whole number from 8 through 256, inclusive. | Number of samples in an audio transfer batch. At 8 kHz, 32 samples represent 4 ms. |

## calibration

| Key | Default | Allowed values | What it does |
|---|---|---|---|
| `probe_duration_ms` | `300` | Whole number from 50 through 5000 ms, inclusive. | Duration of the known test sound used to estimate the speaker-to-microphone model. |
| `correlation_threshold` | `0.3` | Greater than 0 and less than 1; also greater than `tracking.watch_correlation_threshold`. | Threshold intended to request recalibration when the model appears to have drifted. |
| `correlation_window_ms` | `1000` | Whole number from 100 through 10000 ms, inclusive. | Time window used to measure the correlation signal. |
| `backstop_interval_ms` | `600000` | Unsigned 32-bit whole number (0 through 4294967295); must be at least `cooldown_ms`. | Maximum planned interval between calibrations; the default is 10 minutes. |
| `cooldown_ms` | `30000` | Unsigned 32-bit whole number (0 through 4294967295); must not exceed `backstop_interval_ms`. | Minimum planned wait between calibrations and the fault-recovery waiting period; the default is 30 seconds. |

## tracking

| Key | Default | Allowed values | What it does |
|---|---|---|---|
| `initial_freq_hz` | `[60.0]` | Array of 1 to 4 frequencies; each greater than 0 and less than 4000 Hz. | Starting hints for the frequency trackers. The array length determines the number of active tones. |
| `max_drift_rate_hz_per_sec` | `2.0` | Greater than 0; validation sets no upper bound. | Limits how quickly a tracked frequency is allowed to change. |
| `watch_correlation_threshold` | `0.15` | Greater than 0 and strictly less than `calibration.correlation_threshold`. | Earlier warning threshold intended to mark frequency tracking as low-confidence. |

## Rules involving multiple settings

- `backstop_interval_ms >= cooldown_ms`: the maximum calibration interval cannot
  be shorter than the minimum wait. Equality is accepted.
- `watch_correlation_threshold < correlation_threshold`: the warning must occur
  before the recalibration trigger. Equality is rejected.
- Only active tone slots are validated. Unused slots can remain zero.

## Current reader behavior

The supported input ranges are listed above. The current reader is permissive:
wrong-type or absent scalar fields keep defaults; an empty frequency array keeps
the default tone count; arrays longer than four are truncated. These are recovery
behaviors, not recommended input formats. Use finite numbers and whole numbers
where indicated rather than relying on the reader's numeric casts.

The sample rate is fixed at 8000 Hz in code and is not a JSON setting. There is
no independent `num_tones` JSON key: `initial_freq_hz` supplies that count.
Changing model length or audio batch size will require appropriate buffer
reinitialization when runtime settings updates are implemented.
