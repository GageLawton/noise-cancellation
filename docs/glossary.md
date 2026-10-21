# Audio and ANC glossary

Short definitions for the audio, signal-processing, and embedded-systems terms used in the project.

## Active noise cancellation (ANC)

ANC reduces an unwanted sound by generating a controlled speaker signal that makes the sound measured at the microphone smaller. This project focuses on narrowband, tonal cancellation rather than broadband room cancellation; see the [design scope](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#1-scope).

## BCLK and WS

**BCLK** (bit clock) times the individual bits on an I2S audio link. **WS** (word select, also called LRCLK) marks the boundary between audio words or channels; together they let an I2S microphone or amplifier interpret the serial data stream. The project uses an I2S microphone and amplifier as described in the [hardware design](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#3-hardware-per-node).

## Clipping

Clipping happens when a signal asks for a level beyond what the output can produce, so the tops of the wave are cut flat. It sounds harsh and can stress the speaker, so the project caps every output sample at a set ceiling and counts each time that happens; see [`output_limiter.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/dsp_core/include/anc/output_limiter.hpp).

## Coordinator

The coordinator is the role that collects the status of every node, so one place knows how the whole system is doing. Node 1 plays it for now, alongside its own node runtime, and the role does not move when more nodes arrive; see the [system architecture](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#2-system-architecture) and [`coordinator.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/coordinator/include/anc/coordinator.hpp).

## Correlation (normalized)

Normalized correlation measures how alike two signals are on a scale from -1 to +1: near 0 means unrelated, near +1 means the same shape, and near -1 means the same shape flipped. The project uses it to spot a copy of its own speaker output left in the estimated noise, which means the speaker-to-mic model is stale; see [`rolling_correlation.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/alignment/include/anc/rolling_correlation.hpp).

## Crest factor

The crest factor is the peak level divided by the RMS level, usually given in dB. A sine is about 3 dB and a square wave is 0 dB; a high value means short spikes that come close to clipping even when the average level is modest; see [`peak.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/dsp_core/include/anc/peak.hpp).

## dBFS

Decibels relative to full scale measure a digital audio level against the largest value the format can represent. `0 dBFS` is full scale; ordinary signal levels are negative, so a level of `-6 dBFS` is lower than `-3 dBFS`.

## DC offset

A DC offset is a constant value added to every sample, so the signal sits above or below zero instead of centred on it. Microphones often have a small one; the project removes it with a filter that cuts frequencies below about 6 Hz while keeping a 60 Hz hum; see [`dc_blocker.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/dsp_core/include/anc/dc_blocker.hpp).

## Decimation and upsampling

Decimation lowers the sample rate (for example 16 kHz to 8 kHz) by low-pass filtering and then keeping every second sample; upsampling raises it by inserting new samples and smoothing them. They let the I2S link run faster than the 8 kHz the DSP uses if the hardware needs it; see [`decimator2x.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/dsp_core/include/anc/decimator2x.hpp) and [`upsampler2x.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/dsp_core/include/anc/upsampler2x.hpp).

## Divergence

An adaptive filter diverges when its weights grow instead of settling, so it makes the noise louder rather than quieter. The project watches the leftover (residual) power and treats a hard limit or steady growth as divergence, so the node can mute and recover; see [`divergence_detector.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/dsp_core/include/anc/divergence_detector.hpp).

## DMA

Direct memory access lets a peripheral move data to or from memory without the CPU copying every sample. The I2S DMA buffer therefore lets the audio task process blocks while the peripheral continues transferring samples; see the [task and concurrency design](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#54-task--concurrency-design-freertos).

## FxLMS

Filtered-x Least Mean Squares is the adaptive algorithm used to tune the cancellation signal. It filters the reference through an estimate of the speaker-to-microphone path before updating the weights, which accounts for the physical path between the filter output and the measured error; see [§6.1](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#61-fxlms-filtered-x-least-mean-squares).

## Goertzel algorithm

The Goertzel algorithm measures the level of one frequency in a block of samples without computing a full FFT. It is cheap when only a few frequencies matter, such as checking how loud a 60 Hz hum is; see [`goertzel.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/dsp_core/include/anc/goertzel.hpp).

## I2S

I2S is a synchronous serial interface for digital audio samples. It carries the sample data together with clock signals such as BCLK and WS; this project uses it for the MEMS microphone input and class-D amplifier output described in the [hardware design](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#3-hardware-per-node).

## Impulse response

An impulse response is what comes out of a system when a single click goes in: a list of values (taps) showing how much of the input arrives after each delay. The project measures the speaker-to-microphone impulse response by playing white noise, and its biggest tap gives the acoustic delay; see [`ir_estimator.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/calibration/include/anc/ir_estimator.hpp) and [`ir_analysis.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/calibration/include/anc/ir_analysis.hpp).

## NLMS and step size

Normalized Least Mean Squares (NLMS) is a way of updating adaptive filter weights a little after every sample, scaled by the signal's power so the update behaves the same at any volume. The step size (`normalized_step_size` in the config) sets how big each update is: larger learns faster but can become unstable; see [`fxlms_canceller.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/dsp_core/include/anc/fxlms_canceller.hpp).

## Nyquist frequency

The Nyquist frequency is half the sample rate and is the highest frequency that can be represented without aliasing in an ideally sampled signal. At the project's 8 kHz sample rate, the Nyquist frequency is 4 kHz.

## Phase-locked loop (PLL)

A phase-locked loop keeps its own oscillator in step with an incoming tone by measuring the phase difference and nudging its frequency to shrink it. The project uses one per tracked tone so the canceller follows a hum that drifts, such as mains frequency wander or a fan changing speed; see [`pll.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/alignment/include/anc/pll.hpp).

## Reference signal

A reference signal is the input correlated with the disturbance that an adaptive controller uses to decide what to generate. Here the reference is a synthesized sine/cosine pair at each tracked tone, not a signal from a dedicated acoustic reference microphone; see the [signal notation](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#60-notation) and [FxLMS design](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#61-fxlms-filtered-x-least-mean-squares).

## Ring buffer

A ring buffer is a fixed-size circular queue for streaming data. A producer writes new samples at one end while a consumer reads older samples at the other, wrapping around when either reaches the end; the project uses lock-free ring buffers between real-time audio tasks.

## Sample rate

The sample rate is the number of audio samples captured or produced per second, measured in hertz. The design currently uses 8 kHz, so one sample represents 125 microseconds of audio time; see [§6.1](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#61-fxlms-filtered-x-least-mean-squares).

## Secondary path

The secondary path is the transfer function from the speaker, through the amplifier and acoustic environment, to the microphone. FxLMS estimates this path as `S(z)` so its updates account for how the generated signal actually returns to the error microphone; see the [signal notation](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#60-notation).

## Sync tick

A sync tick is a pulse on the wired sync line between nodes that keeps them in step, so every board agrees on when a block starts. It is phase 1 of the timing plan; later phases may send it over radio instead. See [timing synchronization](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#4-timing-synchronization) and [`sync_tick.hpp`](https://github.com/GageLawton/noise-cancellation/blob/main/components/transport/include/anc/sync_tick.hpp).

## Telemetry

Telemetry is the one-line status the firmware prints over the serial port every second, such as the state, residual level and clip count. It lets you watch a node from a laptop without a debugger, and [`telemetry_to_csv.py`](https://github.com/GageLawton/noise-cancellation/blob/main/tools/telemetry_to_csv.py) turns a saved log into a table; see [telemetry and logging](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#58-telemetry--logging).

## Tonal noise

Tonal noise is unwanted sound concentrated around one or more narrow frequency bands, such as HVAC hum or transformer buzz. It is the target of this project's narrowband ANC system; see the [design scope](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#1-scope).
