# Sync tick between two boards

A small standalone ESP-IDF app that checks the sync tick between two boards
(issue #302), after the one-board loopback in [`../sync_loopback`](../sync_loopback)
works. It uses the project's real `GpioSyncTick` from `components/transport`.

Both boards run the same app: each one sends 1000 ticks, one every 100 ms, then
pauses 5 seconds and starts again, and listens for ticks the whole time. So there
is no sender or receiver build to choose.

## Wiring

| Board 1 | Board 2 | |
|---|---|---|
| GPIO15 (sync out) | GPIO16 (sync in) | the sync tick |
| GND | GND | shared ground (required) |

To check the other direction too, add a second wire from board 2 GPIO15 to
board 1 GPIO16. See the "Sync line" section of [`docs/hardware.md`](../../docs/hardware.md).

## Build and flash

From the ESP-IDF 5.3 shell, in this folder, flash each board in turn (plug in one
at a time, or use each board's own port):

bash:

```bash
idf.py set-target esp32s3
idf.py -p /dev/ttyUSB0 build flash
idf.py -p /dev/ttyUSB1 flash monitor
```

PowerShell (ESP-IDF CMD or PowerShell shortcut):

```powershell
idf.py set-target esp32s3
idf.py -p COM5 build flash
idf.py -p COM6 flash monitor
```

Open the monitor on board 2 (the one whose GPIO16 is wired). Press `Ctrl+]` to
leave the monitor.

## Output

About every 105 seconds, the receiving board prints one line per burst:

```
sync_two_boards: sending on GPIO15, listening on GPIO16, 1000 ticks every 100 ms
sent burst 1: 1000 ticks
received 1000/1000, largest gap 100123 us, gaps over 150000 us: 0
```

A good result is `1000/1000`, a largest gap close to 100000 us (100 ms), and no
gaps over 150000 us. A board with nothing wired to its GPIO16 prints
`no ticks heard on GPIO16 ...` every 10 seconds instead, which is expected on
board 1 when there is only one wire.

The counts start when the app starts, so reset both boards together (or ignore
the first line) to get a full burst.

(The numbers above show the format only; they aren't measured.)
