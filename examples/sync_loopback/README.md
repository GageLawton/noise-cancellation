# Sync tick loopback example

A small standalone ESP-IDF app that checks the sync tick on one board before two
boards share a sync wire (issue #267). It uses the project's real `GpioSyncTick`
from `components/transport`.

## Wiring

One jumper wire from **GPIO15** (`kPinSync1`, the output) to **GPIO16**
(`kPinSync2`, the input). Nothing else needs to be connected.

## Build and flash

From the ESP-IDF 5.3 shell, in this folder:

```
idf.py set-target esp32s3
idf.py -p COM5 build flash monitor
```

Use your board's port: `COM5` on Windows, something like `/dev/ttyUSB0` on Linux or
`/dev/cu.usbserial-0001` on macOS. Press `Ctrl+]` to leave the monitor.

## Output

Every few seconds the app sends 1000 ticks to itself and prints one line:

```
sync_loopback: GPIO15 -> GPIO16, 1000 rounds
run 1: received 1000/1000, worst round trip 45 us, mean 21 us
```

With the jumper in place it should say `1000/1000`. Pull the jumper out while it
runs and the next line should say `received 0/1000 (no jumper?)`. Each round waits
up to 10 ms for its tick, so a run with no jumper takes about 11 seconds.

(The numbers above show the format only; they aren't measured.)
