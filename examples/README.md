# Examples

Small standalone ESP-IDF apps that use the project's real components to check
one piece of hardware at a time. Each is built and flashed on its own, separately
from the main firmware in `main/`, and CI builds every one of them.

| Example | What it checks | Hardware |
|---|---|---|
| [`i2s_bringup/`](i2s_bringup/) | The breadboard wiring: prints the mic level and plays test tones through the amp on the shared I2S bus. Use it first, before the full firmware. | The board with the INMP441 mic, MAX98357A amp and speaker |
| [`sync_loopback/`](sync_loopback/) | The sync tick on one board: sends 1000 ticks to itself and counts how many come back. | One board and one jumper wire (GPIO15 to GPIO16) |
| [`sync_two_boards/`](sync_two_boards/) | The sync tick between two boards: both run the same app, and the receiving board counts the ticks and the largest gap. | Two boards, a sync wire and a shared GND |

Each folder's own README has the wiring, settings and the output to expect.

## Build and flash

From the ESP-IDF 5.3 shell, in the example's folder. Use your board's port for `PORT`.

bash:

```bash
cd examples/sync_loopback
idf.py set-target esp32s3
idf.py -p PORT build flash monitor
```

PowerShell:

```powershell
cd examples\sync_loopback
idf.py set-target esp32s3
idf.py -p PORT build flash monitor
```

Press `Ctrl+]` to leave the monitor.
