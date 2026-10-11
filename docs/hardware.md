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

## Safety notes

- The microphone runs on 3.3 V. Never connect it to 5 V.
- Tie the microphone's L/R pin to GND.
- Connect the speaker across the amplifier's + and − terminals only; neither terminal is ground.
