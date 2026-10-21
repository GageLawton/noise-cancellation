# Hardware wiring

This guide documents the breadboard wiring for the first audio-plumbing prototype.

## Parts

- ESP32-S3-DevKitC-1
- INMP441 I2S microphone breakout
- MAX98357A I2S amplifier breakout
- Small 4 to 8 Ω speaker
- Breadboard
- Jumper wires

## Pin map

| ESP32-S3 pin | Signal | INMP441 mic | MAX98357A amp |
|---|---|---|---|
| GPIO 4 | I2S bit clock (BCLK) | SCK | BCLK |
| GPIO 5 | I2S word select (WS) | WS | LRC |
| GPIO 6 | I2S data out, to amp |  | DIN |
| GPIO 7 | I2S data in, from mic | SD |  |
| 3V3 | 3.3 V | VDD |  |
| 5V | 5 V from USB |  | VIN |
| GND | ground | GND, L/R | GND |

## Sync line (M7)

The sync line keeps several boards in step: one board sends a short pulse (a sync tick) and the others wait for it, so they all agree on timing. It is phase 1 of the [timing plan](design.md#4-timing-synchronization), a plain wire between boards. One board's GPIO15 (`kPinSync1`, out) drives the other boards' GPIO16 (`kPinSync2`, in); both pins are set in [`pins.hpp`](../components/anc_hal/include/anc/pins.hpp).

| Board 1 pin | Board 2 pin | Signal |
|---|---|---|
| GPIO 15 (sync out) | GPIO 16 (sync in) | sync tick |
| GND | GND | shared ground (required) |

With a single board, a jumper wire from GPIO15 to GPIO16 lets the board hear its own tick; this is the loopback bench check in [#267](https://github.com/GageLawton/noise-cancellation/issues/267).

Both boards run at 3.3 V logic. Never connect a 5 V pin to a GPIO.

## Safety notes

- The microphone runs on 3.3 V. Never connect it to 5 V.
- Tie the microphone's L/R pin to GND.
- Connect the speaker across the amplifier's + and − terminals only; neither terminal is ground.
