# Benchmark Results

This page serves as a central repository for collecting hardware measurement data. When submitting benchmarking data, please ensure each entry includes the **date**, the **commit hash tested**, and the **exact hardware configuration used**.

## Bench Check Results

### M2 Audio Loop
* **Issue Link:** See [Bench check #92](https://github.com)
* **Description:** Run the M2 audio loop for 10 minutes without dropouts.

| Date | Commit | Hardware Used | Run Length | Blocks/s | Filter/s | Align/s | Underruns | Drops | Mic dBFS (Quiet) | Mic dBFS (Clap) |
| :--- | :----- | :------------ | :--------- | :------- | :------- | :------ | :-------- | :---- | :--------------- | :-------------- |
|      |        |               |            |          |          |         |           |       |                  |                 |

---

### M3 Test Tone
* **Issue Link:** See [Bench check #100](https://github.com)
* **Description:** Measure the test tone with the measurement mic.

| Date | Commit | Hardware Used | Target Hz | Measured Hz | Audible (Yes/No) |
| :--- | :----- | :------------ | :-------- | :---------- | :--------------- |
|      |        |               |           |             |                  |

---

### M4 Speaker-to-Mic Delay
* **Issue Link:** See [Bench check #110](https://github.com)
* **Description:** Confirm 128 taps covers the measured speaker-to-mic delay.

| Date | Commit | Hardware Used | Mic Distance | Peak Index | Peak Value | Delay (ms) | Settled Before Tap 128 (Yes/No) |
| :--- | :----- | :------------ | :----------- | :--------- | :--------- | :--------- | :------------------------------ |
|      |        |               |              |            |            |            |                                 |

---

### M5 Cancellation
* **Issue Link:** See [Bench check #122](https://github.com)
* **Description:** Measure the first real cancellation in dB.

| Date | Commit | Hardware Used | Frequency | Depth dB | Time to Converge |
| :--- | :----- | :------------ | :-------- | :------- | :--------------- |
|      |        |               |           |          |                  |

---

## Hardware Environment Reference

Please use this baseline checklist when specifying your test environment. Refer to [docs/hardware.md](hardware.md) for core parts specifications.

* **Core System / Microcontroller:** [As specified in docs/hardware.md]
* **Speaker Model:** _________________ *(Specify exact model used)*
* **Measurement Microphone:** _________________ *(Specify exact model used)*
