# Tonal ANC — Engineering Design Decisions

## 1. Scope
- Narrowband/tonal cancellation, not broadband
- Broadband open-room ANC requires a multi-mic/speaker spatial array — out of scope for single mic/speaker hardware

## 2. System Architecture
- Coordinated MIMO multi-node system (not independent per-zone nodes)
- Node #1 hosts both the coordinator role and the node runtime
- Future nodes implement node runtime only; coordinator role does not move

## 3. Hardware (per node)
- MCU: ESP32-S3
- Mic: single I2S MEMS mic (INMP441-class) — no dedicated reference mic; reference signal is synthesized, not acoustically sourced
- Output: I2S class-D amp (MAX98357A-class) + small full-range driver
- Sync I/O: spare GPIO pair reserved for wired sync line

## 4. Timing Synchronization
- Phase 1 (now): wired GPIO sync line, sub-µs accuracy
- Phase 2: ESP-NOW / custom WiFi protocol, target tens-of-µs accuracy
- Phase 3 (if needed): IEEE 1588 PTP over Ethernet

## 5. Software Architecture

### 5.1 Layers (top to bottom)
1. Coordinator role — owns shared clock/sync, collects signal/state from all nodes, computes/distributes filter output
2. Node runtime — local ADC/DAC I/O, exposes it over sync/comms link
3. DSP core — FxLMS adaptive filter, hardware- and network-agnostic
4. HAL layer — ADC/DAC/mic/speaker I/O wrapper
5. Sync/transport shim — wired GPIO today; only layer rewritten for wireless sync

### 5.2 Features
- Real-time single-channel audio capture (mic) and single-channel audio output (speaker)
- Synthesized reference signal (sine/cosine at tracked frequency) — no acoustic reference mic
- Ambient signal reconstruction: `d_hat(n) = e(n) - S(z)·y(n)`, run continuously
- Continuous frequency tracking: bank of PLL/frequency-locked loops (one per tone/harmonic) running on `d_hat(n)`
- Self-lock safeguard (two mechanisms): (1) rate limiting — tracked frequency change per update capped to the physically plausible drift rate of the noise sources, rejects sudden self-lock; (2) correlation-based confidence gating — reuses the `d_hat(n)`/`y(n)` correlation signal at a lower "watch" threshold than the calibration trigger to mark the current lock low-confidence when leakage is contaminating the tracker input, catching persistent small-bias self-lock that rate limiting can't
- FxLMS adaptive filtering loop, driven by the synthesized reference
- Secondary path calibration: full version at first boot (genuine silent window, no `S(z)` yet); periodic muted re-check thereafter to validate `S(z)` hasn't drifted
- Output safety limiter (hard soft-clip, independent of filter math)
- Divergence detection and automatic fault recovery
- Wired sync tick generation/consumption (GPIO)
- Node status reporting (convergence state, residual power, tracked frequency per tone)
- Runtime config interface (serial/UART, backed by a JSON file on flash — filter length, block size, calibration/tracking thresholds, step size; no recompilation needed for any of it)
- Coordinator stub: single-node passthrough for v1, extension point for multi-node distribution later

### 5.3 State Machine
```
BOOT → INIT_HAL → CALIBRATE_SECONDARY_PATH → RUN ──(periodic timer)──► RECHECK_SECONDARY_PATH ──┐
                        ↑                       │  ↑                                            │
                        └──── FAULT ◄────────────┘  └────────────────────────────────────────────┘
                                 │
                            SAFE_MUTE
```
- `BOOT`: power-on, load config (JSON from flash, falling back to compiled-in defaults if missing/corrupt)
- `INIT_HAL`: bring up I2S in/out, GPIO sync
- `CALIBRATE_SECONDARY_PATH`: (first boot only) genuine silent window — no `S(z)` exists yet to reconstruct anything; play probe signal, compute FIR estimate, load into FxLMS
- `RUN`: adaptive filtering active; reference synthesis, ambient reconstruction, and frequency tracking all running continuously
- `RECHECK_SECONDARY_PATH`: brief muted window on a periodic timer; re-verifies `S(z)` hasn't drifted, updates it, returns to `RUN` — same mechanism as first-boot calibration, just triggered on a timer instead of once
- `FAULT`: entered on divergence detection or output clipping threshold exceeded; mutes output, logs cause
- `SAFE_MUTE`: output forced silent; returns to `CALIBRATE_SECONDARY_PATH` on manual reset or after fault cooldown

### 5.4 Task / Concurrency Design (FreeRTOS)
**Core 0 — real-time, highest priority**
- `AudioIOTask`: I2S DMA-driven capture/output, feeds/drains ring buffers, no blocking calls
- `FilterTask`: pulls reference+error samples from ring buffer, runs FxLMS update per block, pushes output samples

**Core 1 — non-real-time**
- `AlignmentTask`: owns ambient reconstruction (EstimatedNoise), the PLL frequency-tracker bank, and the calibration-trigger correlation check — grouped here because none of it tolerates the jitter budget of `FilterTask`, and none of it is audio-critical if occasionally late or dropped
- `CoordinatorTask`: stub for v1 (local passthrough); future home for multi-node aggregation
- `TelemetryTask`: periodic (1Hz) status log over UART
- `ConfigTask`: handles serial command interface (calibration trigger, param changes)

**Inter-task communication:** lock-free ring buffers between `AudioIOTask` ↔ `FilterTask`; a shared status struct (single-writer `FilterTask`, multi-reader `TelemetryTask`/`CoordinatorTask`) for state/metrics. `FilterTask` → `AlignmentTask`: non-blocking push of `{MicSignal, SynthOutput}` per cycle into a lock-free SPSC ring buffer; on overflow, drop the oldest sample rather than block `FilterTask` — core 0 must never stall waiting on core 1.

### 5.5 Module Interfaces
Conceptual interface sketch. **Superseded by Section 10.3**, which defines the actual C++ types.
```c
// HAL layer
hal_audio_init();
hal_audio_read_block(float* mic_buf, size_t n);  // normalized [-1.0, 1.0]; 24-in-32-bit I2S frame converted here
hal_audio_write_block(float* out_buf, size_t n); // normalized [-1.0, 1.0]; converted to output bit width here
hal_sync_gpio_init();
hal_sync_wait_tick();
hal_sync_signal_tick();

// Reference synthesis
refgen_set_frequency(uint8_t tone_idx, float freq_hz);
refgen_next_sample(uint8_t tone_idx); // -> float, sine/cosine pair at tracked frequency

// Ambient reconstruction
reconstruct_sample(float mic_signal, float synth_output_sample); // -> EstimatedNoise, applies SecondaryPath

// Frequency tracker (bank, one instance per tone)
freq_tracker_init(uint8_t tone_idx, float initial_freq_hz);
freq_tracker_update(uint8_t tone_idx, float d_hat_sample); // -> updated freq estimate
freq_tracker_get_status(uint8_t tone_idx); // -> { freq_hz, locked }

// DSP core
fxlms_init(FxlmsConfig cfg);
fxlms_set_secondary_path(float* coeffs, size_t n);
fxlms_process_sample(float ref_sample, float err_sample, float* out_sample);
fxlms_get_status(); // -> { converged, residual_power, coeff_norm }

// Calibration
calibrate_secondary_path(); // blocking, muted window — used at first boot AND periodic recheck

// Node runtime
node_init();
node_get_status(); // -> NodeStatus

// Coordinator (v1: local stub)
coordinator_init();
coordinator_register_node(node_id);       // stub, future
coordinator_distribute_filter_update();   // stub, future

// Sync/transport shim
transport_send_status(NodeStatus status);
transport_receive_config(NodeConfig cfg);
```

### 5.6 Data Structures / Message Formats
Defined now even though v1 is single-node, so the coordinator↔node interface doesn't change shape when node #2 arrives.

Node identity: `node_id` (protocol/machine identity) is derived from the ESP32-S3's factory-burned MAC address at boot — guaranteed globally unique, no provisioning step, no DIP switches or hand-assigned IDs. `node_name` (Section 5.7 JSON config) is a separate, optional human-readable label for logs/display only — never used in protocol logic, so a typo there can't cause an identity collision.

```c
typedef struct {
    uint64_t node_id;        // derived from factory MAC, guaranteed unique
    bool     converged;
    float    residual_power;
    float    tracked_freq_hz[MAX_TONES];
    bool     freq_locked[MAX_TONES];
    uint32_t timestamp;
} NodeStatus;

typedef struct {
    uint16_t secondary_path_taps;   // SecondaryPath model length, default 128
    float    normalized_step_size;  // NormalizedStepSize, dimensionless, starting value 0.1
    bool     trigger_recalibration;
} NodeConfig;

// Future wireless sync payload (Phase 2) — GPIO tick carries no payload in Phase 1
typedef struct {
    uint64_t node_id;        // derived from factory MAC, guaranteed unique
    uint64_t timestamp_us;
    uint32_t sequence_number;
} SyncMessage;
```

### 5.7 Configuration Management
- Storage: JSON file on a LittleFS partition on internal flash
- Parsing: cJSON (bundled with ESP-IDF)
- Boot: `BOOT` state loads JSON into in-memory `NodeConfig`; missing/corrupt file falls back to compiled-in defaults and writes them out fresh
- Runtime updates via serial command interface (`ConfigTask`): get/set individual fields by name; a set updates the in-memory value immediately and persists the whole file back to flash — no recompilation required for any value in the file
- `secondary_path_taps` and `dma_block_samples` changes require a defined buffer reinit step (not a full reboot) since they affect allocation sizes
- `ε` (NLMS regularization constant) stays a compile-time constant — a numerical-safety floor, not a bench-tuning parameter
- Config fields (all runtime-adjustable via the JSON file):
```json
{
  "identity": {
    "node_name": "node-1"
  },
  "dsp": {
    "secondary_path_taps": 128,
    "normalized_step_size": 0.1,
    "dma_block_samples": 32
  },
  "calibration": {
    "probe_duration_ms": 300,
    "correlation_threshold": 0.3,
    "correlation_window_ms": 1000,
    "backstop_interval_ms": 600000,
    "cooldown_ms": 30000
  },
  "tracking": {
    "initial_freq_hz": [60.0],
    "max_drift_rate_hz_per_sec": 2.0,
    "watch_correlation_threshold": 0.15
  }
}
```

### 5.8 Telemetry & Logging
- 1Hz status log over UART: residual error power, filter coefficient norm, convergence flag, current state
- Output clipping events logged with timestamp and counter

### 5.9 Fault Handling
- Divergence detection: residual error power monitored per block; threshold-exceeded or sustained growth over N blocks → `FAULT`
- On `FAULT`: mute output immediately, log cause, hold in `SAFE_MUTE`
- Recovery: manual reset or automatic retry into `CALIBRATE_SECONDARY_PATH` after cooldown period

### 5.10 Repo / Module Layout
See Section 10.2 for the full monorepo layout and Section 10.1 for language conventions.

## 6. Signal Processing

### 6.0 Notation
| Symbol | Name | Meaning |
|---|---|---|
| `e(n)` | MicSignal | Raw sample read from the mic — everything the mic actually hears |
| `d(n)` | AmbientNoise | True ambient noise alone (as if the canceller weren't running) — never directly measurable |
| `y(n)` | SynthOutput | Signal generated by reference synthesis, sent to the speaker |
| `S(z)` | SecondaryPath | Speaker-to-mic transfer function |
| `y'(n) = S(z)·y(n)` | AcousticOutput | SynthOutput after SecondaryPath — the system's own output as heard at the mic |
| `d_hat(n) = e(n) - y'(n)` | EstimatedNoise | Reconstructed AmbientNoise: MicSignal with AcousticOutput subtracted out |

FxLMS adapts on **MicSignal**. The frequency tracker and the calibration-trigger correlation check both run on **EstimatedNoise**.

### 6.1 FxLMS (Filtered-x Least Mean Squares)
The adaptive filtering algorithm driving cancellation. A variant of standard LMS built for control loops where the filter's output doesn't reach the error measurement directly — it passes through an intervening physical path first. That's exactly this system's situation: SynthOutput only reaches the mic after passing through SecondaryPath, and adapting weights without accounting for that would cause instability.

**Difference from plain LMS:** LMS updates weights using the raw reference signal directly against the error. FxLMS instead filters the reference through an estimate of the intervening path (SecondaryPath) before using it in the weight update — the "filtered reference" is where the name comes from.

**Update rule (this project's naming) — Normalized LMS:**
```
RunningPower(n)       = (1-λ)·RunningPower(n-1) + λ·FilteredReference(n)²
EffectiveStepSize(n)  = NormalizedStepSize / (ε + RunningPower(n))
FilteredReference(n)  = SecondaryPath · ReferenceBasis(n)
FilterWeights(n+1)    = FilterWeights(n) + EffectiveStepSize(n) · MicSignal(n) · FilteredReference(n)
SynthOutput(n)        = FilterWeights(n) · ReferenceBasis(n)
```
Normalized over a fixed step size because FilteredReference's power shifts across calibration events and as tracked tones/harmonics change — a fixed µ tuned for one amplitude either goes unstable or needlessly slow when that amplitude changes. Normalizing makes the tunable constant dimensionless and amplitude-independent.

| Symbol/Name | Meaning |
|---|---|
| ReferenceBasis(n) | Sine/cosine pair at the currently tracked frequency (from reference synthesis) — raw, unweighted reference FxLMS adapts against |
| FilterWeights | Adaptive amplitude/phase weights FxLMS tunes each sample — this is "the filter" |
| FilteredReference | ReferenceBasis passed through SecondaryPath, used only for the weight-update math, not for driving the speaker |
| RunningPower | Exponential moving average of FilteredReference², updated every sample |
| NormalizedStepSize | Dimensionless tuning constant, stability-bounded in (0, 2). **Starting value: 0.1** — conservative fraction of the stability ceiling; tune up (0.3–0.5) for faster convergence or down (0.05) if residual jitter appears |
| ε | Small fixed regularization constant (e.g. 1e-6) preventing division blowup when RunningPower is near zero (startup, post-mute) |
| MicSignal | The error signal FxLMS minimizes |

**Why SecondaryPath calibration exists at all:** FxLMS structurally requires a SecondaryPath estimate to compute FilteredReference. Without one, cancellation doesn't just get less accurate — it can diverge outright.

- Sample rate: 8kHz
- **FxLMS FilterWeights: 2 per tracked tone** (amplitude/phase of the synthesized sinusoid) — small by design, a consequence of the synthesized-reference decision, not a general-purpose FIR
- **SecondaryPath model length: 128 taps (16ms at 8kHz), separate parameter from FxLMS's weights.** Must be long enough to represent the true round-trip loop delay (mic→compute→DAC→amp→acoustic travel), or FxLMS and EstimatedNoise reconstruction both become inaccurate regardless of other tuning. See Section 6.2 (Latency Budget) for sizing rationale — 128 is a safe starting default, verified against the calibration probe's actual measured impulse response once hardware exists, not assumed correct
- Reference signal: synthesized sine/cosine per tracked tone, not acoustically sourced — no dedicated reference mic
- **Calibration vs alignment (terminology):**
  - **Calibration** — discrete, muted event that measures SecondaryPath (`S(z)`). Responds to an actual physical/location/environment change. States: `CALIBRATE_SECONDARY_PATH` (first boot, no SecondaryPath exists yet) and `RECHECK_SECONDARY_PATH` (triggered recalibration, same mechanism).
  - **Alignment** — continuous, always-running background process, never mutes output. Two parts: (1) correlation check between EstimatedNoise (`d_hat(n)`) and SynthOutput (`y(n)`), which detects that SecondaryPath has gone stale and triggers a calibration event; (2) the PLL frequency-tracker bank, which continuously tracks tone frequency on EstimatedNoise. Alignment detects and adapts in real time; calibration is the corrective action alignment triggers when needed.
- Ambient signal reconstruction: `EstimatedNoise = MicSignal - SecondaryPath · SynthOutput`, computed continuously (part of alignment)
- Frequency estimation: bank of PLL/frequency-locked loops (one per tone/harmonic), run continuously on EstimatedNoise as part of alignment — chosen over FFT peak-pick and adaptive notch for continuous (non-windowed) tracking and lowest compute cost
- Calibration trigger: primarily correlation-based (rolling normalized cross-correlation between EstimatedNoise and SynthOutput over a ~1s window; sustained elevation over several consecutive windows fires a recheck), with a fixed maximum-interval backstop (e.g. every 10 min) for slow drift the correlation check can't see, and a cooldown (e.g. 30s) after each calibration before another can trigger
- Calibration procedure: probe signal → record at mic → compute FIR estimate → load into FxLMS
- **Block size (two separate decisions):**
  - **FxLMS weight-update granularity: per-sample**, not block-averaged. ESP32-S3 FPU makes per-sample updates trivially cheap at 32–64 taps; block-averaging would only trade away tracking responsiveness (works against the PLL bank's frequency-drift tracking) for efficiency headroom that isn't needed
  - **I2S DMA/ISR buffer: 32 samples (4ms at 8kHz)**, tunable toward 16 (tighter latency) or 64 (lower interrupt overhead) based on bench profiling. `FilterTask` still runs 32 individual per-sample FxLMS updates per block — DMA batching is a systems-latency lever, not an adaptation-rate lever. 4ms of buffering is negligible against tonal targets' 5–17ms+ periods (Section 1's ~1ms ceiling only applied to the ruled-out broadband case)
- **Self-lock safeguard:** a stale SecondaryPath leaks a copy of the current AcousticOutput into EstimatedNoise at exactly the tracked frequency, risking the tracker locking onto its own output instead of the real ambient tone. Two guards, applied together:
  - **Rate limiting** — cap per-update frequency change to the physically plausible drift rate of the target noise sources (HVAC hum / mains buzz drift slowly); rejects sudden self-lock
  - **Correlation-based confidence gating** — reuse the EstimatedNoise/SynthOutput correlation signal (already computed for the calibration trigger) at a lower "watch" threshold; above it, mark the current lock low-confidence rather than trusting it fully. Catches persistent small-bias self-lock that rate limiting alone can't
  - Deferred (not v1): active dither — deliberately wobble the output frequency and check whether the "locked" tone follows; real ambient noise won't, leakage will by definition. Adds output complexity/audible cost, only worth it if the two guards above prove insufficient on the bench

### 6.2 Latency Budget
Unlike the broadband case ruled out in Section 1 (~1ms causality ceiling, since an acoustic reference mic requires the reference to arrive before the disturbance it's cancelling), a continuous synthesized-reference tone has no such wall — FxLMS's weights naturally learn to output the correctly phase-shifted tone to compensate for whatever the total loop delay is, as long as it's constant. **Loop latency here is not a hard ceiling — it's an input that SecondaryPath's model length has to be sized to cover.**

Estimated round-trip delay (mic ADC → compute → DAC → amp → acoustic travel to mic):
| Stage | Estimate |
|---|---|
| I2S DMA block | 4ms (32 samples @ 8kHz, Section 6 block size decision) |
| Core 0 compute (FxLMS + HAL) | Negligible (µs) |
| DAC/amp output buffering | Sub-ms, negligible |
| Acoustic travel, speaker→mic (0.5–2m @ ~343 m/s) | 1.5–6ms |
| **Total (estimated)** | **~5–15ms** |

At 8kHz, 15ms is 120 samples — this is why SecondaryPathTaps defaults to 128 (Section 6, above): it needs to fully span the actual delay with margin, or the model can't represent it at all. **This estimate must be confirmed empirically** once hardware exists, by inspecting the calibration probe's measured impulse response directly (Section 9 validation tooling applies here too) — 128 is a safe starting point, not a substitute for measuring the real number.

## 7. Development Environment
- Toolchain: ESP-IDF (not Arduino framework) — raw DMA/I2S timing control required
- Numeric format: float32 throughout the DSP core (FxLMS, reconstruction, PLL bank). ESP32-S3's hardware FPU removes the usual fixed-point speed advantage; float32's relative precision handles the near-equal-magnitude subtraction in EstimatedNoise far better than fixed-point's absolute precision, avoids manual Q-format scaling on StepSize/weights, and keeps DSP-core behavior consistent between device and host for unit testing
- Sample width: mic delivers 24-bit samples in a 32-bit I2S frame; HAL converts to normalized float32 (`[-1.0, 1.0]`) on read and back to the output driver's bit width on write. This is the only place raw hardware sample format is handled — everything above the HAL layer (tasks, DSP core, alignment layer) only ever sees float

## 8. Physical Prototyping
- Platform: ESP32-S3-DevKitC + breakout boards on breadboard; defer custom PCB
- Single mic placed at the target quiet zone (no separate reference mic to place — acoustic feedback path from speaker to reference mic no longer applies)
- Output stage: hard soft-clip/ceiling in DAC path, independent of filter math
- **Power/rail noise (breadboard stage):** because both mic and amp are I2S digital devices, the classic analog "power noise causes audible hum" problem doesn't really apply here — no analog audio signal is ever exposed to the power rail. Remaining risk is minor and handled with standard, beginner-friendly practice:
  1. 0.1µF ceramic decoupling capacitor across power/ground pins on both the mic and amp breakout boards, as close to each chip as possible
  2. One larger capacitor (10–47µF) near the amp's power input, to smooth the current bursts it draws when driving the speaker
  3. Single clean 5V source for everything (a decent wall USB adapter, not a laptop USB port) — don't split power across multiple sources
  4. Keep breadboard wiring short; keep speaker/amp wiring physically separated from mic wiring where possible
  5. Only dig further if audible noise actually shows up after the above — proper PCB-stage power design (planes, filtering) is deferred until the custom PCB step, not needed for the breadboard prototype

## 9. Validation & Testing

### 9.1 Host-Side Unit Testing (off-target)
- DSP core, calibration, and alignment code build and run on a host machine — already possible by construction: those layers are hardware/network-agnostic (Section 5.1) and float32 behaves near-identically host vs device (Section 7)
- Build: second plain-CMake target alongside the ESP-IDF build, compiling the same sources against a test harness — no `#ifdef` branching
- Framework: Unity (ships with ESP-IDF — one framework for host and on-device tests)
- Test inputs: synthetic signals generated in the harness — pure sine at known frequency, sine + noise, deliberately drifting tone, simulated SecondaryPath as a fixed FIR. No hardware needed
- Coverage: FxLMS convergence on a known tone; PLL lock and drift-following; reconstruction recovering a known AmbientNoise given a known SecondaryPath; correlation detector firing on a deliberately mismatched SecondaryPath; self-lock guards rejecting out-of-bounds frequency jumps
- Shares code with the offline analysis tooling in 9.2
- **Not covered off-target:** real acoustic behavior, true SecondaryPath characteristics, I2S/DMA timing, FreeRTOS task interaction, real-time deadline compliance. Host tests validate the math; the bench rig validates the system

### 9.2 Bench Validation (on hardware)
- The system's own mic cannot validate itself — the control loop actively drives what it reads toward zero, so using it to measure cancellation depth is circular
- External calibrated measurement mic, independent recording chain, no part in the control loop — placed at the quiet zone, separate from the system's own mic
- Fixed test rig — cancellation is highly sensitive to geometry (a few cm shifts the whole SecondaryPath); a jig with fixed mounting points for noise source, system mic, speaker, and measurement mic makes results comparable across runs and code changes
- Metric: narrowband dB reduction at the tracked frequency, not raw broadband RMS — `CancellationDepth_dB = 20·log10(RMS_target_freq_off / RMS_target_freq_on)`, measured via FFT bin/bandpass at the target frequency specifically, baseline (muted) vs converged, same rig
- Analysis offline (bench laptop script, not on-device) from recorded external-mic audio — keeps device compute budget untouched, shares signal-processing code with host-side unit testing
- Secondary metric: convergence time from calibration event to steady-state cancellation, captured from the same recording

## 10. Code Structure

### 10.1 Language & Conventions
- **C++17**, ESP-IDF's default toolchain supports it fully
- **No exceptions, no RTTI** (`-fno-exceptions -fno-rtti`) — standard embedded practice, keeps binary size and failure modes predictable
- **No dynamic allocation after init.** Everything the real-time path touches is allocated once during `INIT_HAL` and never again — no `new`/`malloc` inside `FilterTask` or `AudioIOTask`
- **RAII everywhere else** — constructors initialize, destructors clean up. This is the main reason for C++ over C here: PLL instances, FIR filters, and ring buffers each carry independent state, and classes remove the manual init/free bookkeeping that's the usual source of beginner lifetime bugs
- **Virtual functions only at the hardware boundary** (HAL, transport) where host tests need to substitute fakes. DSP core is concrete classes, no vtables — it's already hardware-agnostic by construction, nothing to abstract

### 10.2 Repo Layout (monorepo)
```
anc/
├── CMakeLists.txt              # ESP-IDF top-level build
├── sdkconfig.defaults
├── components/
│   ├── hal/                    # I2S, GPIO sync. Only layer touching ESP-IDF drivers
│   ├── dsp_core/               # FirFilter, FxlmsCanceller, RefGenerator
│   ├── alignment/              # Reconstructor, Pll, DriftDetector, AlignmentEngine
│   ├── calibration/            # probe generation, impulse response → FIR estimate
│   ├── config/                 # JSON load/save, NodeConfig
│   ├── node_runtime/           # state machine, task setup
│   ├── coordinator/            # v1 stub
│   └── transport/              # wired GPIO sync shim
├── main/
│   └── app_main.cpp
├── test/
│   ├── CMakeLists.txt          # host build target (plain CMake, not ESP-IDF)
│   ├── fakes/                  # FakeAudioIo, synthetic signal generators
│   └── test_*.cpp              # Unity tests
├── tools/
│   └── analyze.py              # offline FFT / dB analysis (Section 9.2)
└── docs/
    └── design.md               # this document
```

Host build compiles `dsp_core`, `alignment`, and `calibration` only — those three have no ESP-IDF dependency by design, which is what makes off-target testing free.

### 10.3 Core Types
```cpp
// hal/ — virtual only here, so tests can substitute FakeAudioIo
class IAudioIo {
public:
    virtual ~IAudioIo() = default;
    virtual size_t readBlock(float* buf, size_t n) = 0;        // normalized [-1,1]
    virtual size_t writeBlock(const float* buf, size_t n) = 0;
};
class I2sAudioIo final : public IAudioIo { /* ESP-IDF I2S + DMA */ };

// hal/ — lock-free single-producer/single-consumer, fixed capacity
template <typename T, size_t Capacity>
class SpscRingBuffer {
public:
    bool push(const T& v);      // false if full — caller drops, never blocks
    bool pop(T& out);
};

// dsp_core/
class FirFilter {
public:
    explicit FirFilter(size_t maxTaps);
    void  setCoefficients(const float* coeffs, size_t n);
    float process(float in);
    void  reset();
};

class RefGenerator {            // ReferenceBasis: sine/cosine at tracked frequency
public:
    RefGenerator(float sampleRateHz);
    void setFrequency(float freqHz);
    void next(float& sinOut, float& cosOut);
};

class FxlmsCanceller {
public:
    FxlmsCanceller(size_t numTones, float sampleRateHz);
    void  setSecondaryPath(const float* coeffs, size_t n);
    void  setToneFrequency(size_t toneIdx, float freqHz);
    void  setNormalizedStepSize(float mu);
    float processSample(float micSignal);   // returns SynthOutput
    Status status() const;                  // { converged, residualPower }
};

// alignment/
class Reconstructor {           // EstimatedNoise = MicSignal - SecondaryPath·SynthOutput
public:
    explicit Reconstructor(size_t secondaryPathTaps);
    void  setSecondaryPath(const float* coeffs, size_t n);
    float process(float micSignal, float synthOutput);
};

class Pll {                     // one instance per tracked tone
public:
    Pll(float initialFreqHz, float sampleRateHz);
    void  update(float sample);
    void  setMaxDriftRate(float hzPerSec);   // self-lock rate limiting
    float frequencyHz() const;
    bool  locked() const;
};

class DriftDetector {           // EstimatedNoise vs SynthOutput correlation
public:
    void  update(float estimatedNoise, float synthOutput);
    float correlation() const;
    bool  needsCalibration() const;          // above trigger threshold, sustained
    bool  lowConfidence() const;             // above watch threshold
};

class AlignmentEngine {         // owns Reconstructor + Pll bank + DriftDetector
public:                         // this is what AlignmentTask drives
    void update(float micSignal, float synthOutput);
    // ... accessors for tracked frequencies, calibration request, confidence
};
```

### 10.4 Build Order (skeleton first)
Each milestone is independently runnable — nothing is written that can't be exercised.

| M | Goal | Done when |
|---|---|---|
| **M0** | Both build targets compile empty | `idf.py build` and host `cmake && ctest` both succeed |
| **M1** | Boot path, no audio | Device boots, loads JSON config (falls back to defaults on first flash), spawns all tasks, `TelemetryTask` prints state at 1Hz |
| **M2** | Audio plumbing end-to-end | Mic → `AudioIOTask` → ring buffer → `FilterTask` → ring buffer → speaker. `FilterTask` just passes through or outputs silence. DMA timing verified, no dropouts, `AlignmentTask` receiving samples |
| **M3** | Output path proven | `FilterTask` emits a fixed-frequency sine. Confirms speaker/amp/level/soft-clip path works before any adaptive math exists |
| **M4** | Calibration | Probe signal plays, impulse response captured, FIR estimate computed. Inspect measured delay here to confirm SecondaryPathTaps=128 (Section 6.2 open item) |
| **M5** | Cancellation | `FxlmsCanceller` filled in. Host tests first (synthetic tone, known SecondaryPath), then on-device against a real tone. First real dB measurement on the bench rig |
| **M6** | Alignment | `Reconstructor`, `Pll` bank, `DriftDetector` filled in. Continuous tracking, correlation-triggered calibration, self-lock guards |
| **M7** | Multi-node prep | Wired GPIO sync line, coordinator/peer interface exercised with node #1 alone playing both roles |

M1–M3 are the thin end-to-end skeleton — every task, buffer, and state transition exists and runs before any DSP math is written. M4 onward fills in the boxes.

## 11. Open Decisions
- [ ] Measure the target environment before picking a tone — record ambient audio at the deployment spot with the external mic, FFT it (Section 9.2 tooling), identify actual peaks, harmonics, and tone-vs-noise-floor strength. `initial_freq_hz` defaults to 60.0 (US mains hum) as a tracker starting hint only; the PLL walks to the real tone regardless, and the config value is editable without a rebuild
- [ ] Calibration trigger tuning (correlation threshold, window length, backstop interval, cooldown — all bench-tuning parameters, defaults not yet set)
- [ ] Self-lock safeguard tuning (max plausible drift rate for rate limiting, "watch" correlation threshold — bench-tuning parameters, defaults not yet set)
- [ ] Confirm SecondaryPathTaps=128 default against measured round-trip delay once hardware exists (Section 6.2 estimate, not yet empirically verified)
