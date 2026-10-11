# Audio and ANC glossary

Short definitions for the audio, signal-processing, and embedded-systems terms used in the project.

## Active noise cancellation (ANC)

ANC reduces an unwanted sound by generating a controlled speaker signal that makes the sound measured at the microphone smaller. This project focuses on narrowband, tonal cancellation rather than broadband room cancellation; see the [design scope](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#1-scope).

## BCLK and WS

**BCLK** (bit clock) times the individual bits on an I2S audio link. **WS** (word select, also called LRCLK) marks the boundary between audio words or channels; together they let an I2S microphone or amplifier interpret the serial data stream. The project uses an I2S microphone and amplifier as described in the [hardware design](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#3-hardware-per-node).

## dBFS

Decibels relative to full scale measure a digital audio level against the largest value the format can represent. `0 dBFS` is full scale; ordinary signal levels are negative, so a level of `-6 dBFS` is lower than `-3 dBFS`.

## DMA

Direct memory access lets a peripheral move data to or from memory without the CPU copying every sample. The I2S DMA buffer therefore lets the audio task process blocks while the peripheral continues transferring samples; see the [task and concurrency design](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#54-task--concurrency-design-freertos).

## FxLMS

Filtered-x Least Mean Squares is the adaptive algorithm used to tune the cancellation signal. It filters the reference through an estimate of the speaker-to-microphone path before updating the weights, which accounts for the physical path between the filter output and the measured error; see [§6.1](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#61-fxlms-filtered-x-least-mean-squares).

## I2S

I2S is a synchronous serial interface for digital audio samples. It carries the sample data together with clock signals such as BCLK and WS; this project uses it for the MEMS microphone input and class-D amplifier output described in the [hardware design](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#3-hardware-per-node).

## Nyquist frequency

The Nyquist frequency is half the sample rate and is the highest frequency that can be represented without aliasing in an ideally sampled signal. At the project's 8 kHz sample rate, the Nyquist frequency is 4 kHz.

## Reference signal

A reference signal is the input correlated with the disturbance that an adaptive controller uses to decide what to generate. Here the reference is a synthesized sine/cosine pair at each tracked tone, not a signal from a dedicated acoustic reference microphone; see the [signal notation](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#60-notation) and [FxLMS design](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#61-fxlms-filtered-x-least-mean-squares).

## Ring buffer

A ring buffer is a fixed-size circular queue for streaming data. A producer writes new samples at one end while a consumer reads older samples at the other, wrapping around when either reaches the end; the project uses lock-free ring buffers between real-time audio tasks.

## Sample rate

The sample rate is the number of audio samples captured or produced per second, measured in hertz. The design currently uses 8 kHz, so one sample represents 125 microseconds of audio time; see [§6.1](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#61-fxlms-filtered-x-least-mean-squares).

## Secondary path

The secondary path is the transfer function from the speaker, through the amplifier and acoustic environment, to the microphone. FxLMS estimates this path as `S(z)` so its updates account for how the generated signal actually returns to the error microphone; see the [signal notation](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#60-notation).

## Tonal noise

Tonal noise is unwanted sound concentrated around one or more narrow frequency bands, such as HVAC hum or transformer buzz. It is the target of this project's narrowband ANC system; see the [design scope](https://github.com/GageLawton/noise-cancellation/blob/main/docs/design.md#1-scope).
