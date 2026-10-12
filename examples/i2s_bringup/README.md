# I2S bring-up example

A small standalone ESP-IDF app that checks the breadboard wiring before the real
M2 audio driver exists (issue #52). It reads the INMP441 mic and prints its level,
and plays test tones through the MAX98357A amp at the same time, on one shared
I2S bus (GPIO 4 BCLK, 5 WS, 6 to the amp, 7 from the mic).

## Build and flash

From the ESP-IDF 5.3 shell, in this folder:

```
idf.py set-target esp32s3
idf.py -p COM5 build flash monitor
```

Use your board's port: `COM5` on Windows, something like `/dev/ttyUSB0` on Linux or
`/dev/cu.usbserial-0001` on macOS. On Windows, `build.bat COM5` does the same thing.
Press `Ctrl+]` to leave the monitor.

## Settings

Run `idf.py menuconfig` and open **I2S bring-up**:

| Setting | Default | What it does |
|---|---|---|
| I2S sample rate | 16000 Hz | Set to 8000 for the 8 kHz test |
| Quiet-room noise floor | -80 dBFS | Placeholder. Measure it and update `main/Kconfig.projbuild` |
| Play a test tone | on | Turn off to measure the mic alone |
| Cycle through the test tones | on | Silence, 60, 120, 180 and 440 Hz, 5 s each |
| Fixed tone frequency | 440 Hz | Used when cycling is off |
| Tone amplitude | 10 % | Keep it low at first |

## Output

```
--- tone: 60 Hz ---
mic:   -71.2 dBFS  ( +8.8 dB above floor)  tone  60 Hz  (raw[0]=0xffe41200)
```

`raw[0]` is one untouched mic sample. If it stays `0x00000000`, no data is arriving
on GPIO 7: check the mic's SD wire and its 3.3 V supply.

## Collecting the issue #52 findings

1. **Noise floor:** in a quiet room, read the level while the tone is `off`. Also note a clap.
2. **Speaker into mic:** compare the level during each tone with the `off` level.
3. **Low tones:** listen during each step and note which of 60, 120, 180 and 440 Hz you can hear.
4. **8 kHz:** set the sample rate to 8000, rebuild, and check the readings still make sense.
