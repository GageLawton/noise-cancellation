#include "anc/node_runtime.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "anc/command_parser.hpp"
#include "anc/config_keys.hpp"
#include "anc/decibels.hpp"
#include "anc/goertzel.hpp"
#include "anc/ir_analysis.hpp"
#include "anc/ir_estimator.hpp"
#include "anc/output_limiter.hpp"
#include "anc/rms.hpp"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace anc {
namespace {

constexpr const char* kTag = "runtime";

// Core assignment: core 0 is reserved for the real-time audio path so
// nothing on core 1 can ever delay it. See design doc section 5.4.
constexpr BaseType_t kRealtimeCore    = 0;
constexpr BaseType_t kNonRealtimeCore = 1;

constexpr uint32_t kTelemetryPeriodMs = 1000;
// Telemetry lines between stack reports (10 s).
constexpr uint32_t kStackReportEvery = 10;

// Smallest free stack the task has ever had, in bytes, or 0 if it never
// started. nullptr is not allowed here: FreeRTOS reads that as "this task".
unsigned stackFree(TaskHandle_t task) {
    return task == nullptr ? 0u
                           : static_cast<unsigned>(uxTaskGetStackHighWaterMark(task));
}

// How long the main task waits for FilterTask to act on a request. A
// block is 4 ms, so these only run out if FilterTask has stopped.
constexpr uint32_t kFilterResponseTimeoutMs = 100;
constexpr uint32_t kFadeTimeoutMs           = 200;  // the fade itself is 50 ms

// A calibration peak below this many times the RMS of all taps is
// indistinguishable from noise (see doCalibrate).
constexpr float kMinPeakToRms = 5.0f;

// Calibration warns when fewer taps than this (8 ms at 8 kHz) follow the
// peak, because the speaker's ring after the peak wouldn't fit.
constexpr std::size_t kMinTailTaps = 64;

// The mic level meter falls 20 dB per second (250 blocks).
constexpr float kMicMeterFallDbPerBlock = 20.0f / 250.0f;

uint32_t nowMs() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}

}  // namespace

uint64_t NodeRuntime::deriveNodeId() {
    uint8_t mac[6] = {};
    esp_err_t err  = esp_efuse_mac_get_default(mac);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "MAC read failed: %s", esp_err_to_name(err));
        return 0;
    }

    uint64_t id = 0;
    for (int i = 0; i < 6; ++i) {
        id = (id << 8) | mac[i];
    }
    return id;
}

void NodeRuntime::transitionTo(NodeState next) {
    if (next == state_)
        return;
    ESP_LOGI(kTag, "%s -> %s", toString(state_), toString(next));
    state_ = next;

    NodeStatus s  = status_.read();
    s.state       = state_;
    s.timestampMs = nowMs();
    status_.publish(s);
}

NodeState NodeRuntime::doBoot() {
    // The version comes from `git describe`, so a pasted log says which
    // code it came from.
    const esp_app_desc_t* app = esp_app_get_description();
    ESP_LOGI(kTag, "firmware %s, built %s %s", app->version, app->date, app->time);

    nodeId_ = deriveNodeId();
    ESP_LOGI(kTag, "node id %012llx", static_cast<unsigned long long>(nodeId_));

    if (!store_.mount()) {
        ESP_LOGE(kTag, "flash unusable, running on compiled-in defaults");
        config_ = NodeConfig::defaults();
    } else {
        store_.load(config_);  // always leaves config_ usable
    }

    ESP_LOGI(kTag, "name=%s taps=%u mu=%.3f block=%u tones=%u", config_.nodeName,
             static_cast<unsigned>(config_.secondaryPathTaps),
             config_.normalizedStepSize, static_cast<unsigned>(config_.dmaBlockSamples),
             static_cast<unsigned>(config_.numTones));

    NodeStatus s{};
    s.nodeId   = nodeId_;
    s.state    = NodeState::Boot;
    s.numTones = config_.numTones;
    for (int i = 0; i < config_.numTones; ++i) {
        s.trackedFreqHz[i] = config_.initialFreqHz[i];
        s.freqLocked[i]    = false;
        // Telemetry shows the starting hint until tracking takes over.
        trackedFreqHz_[i].store(config_.initialFreqHz[i], std::memory_order_relaxed);
        startFreqHz_[i] = config_.initialFreqHz[i];
    }
    scheduler_ = CalibrationScheduler(config_.backstopIntervalMs, config_.cooldownMs);
    outputCeiling_.store(config_.outputCeiling, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        pendingConfig_ = config_;
    }
    s.timestampMs = nowMs();
    status_.publish(s);

    return NodeState::InitHal;
}

NodeState NodeRuntime::doInitHal() {
    if (!audio_.init(defaultI2sSettings())) {
        ESP_LOGE(kTag, "I2S init failed, check the mic and amp wiring");
        faultCause_ = "i2s init";
        return NodeState::Fault;
    }
    tone_.emplace(kSampleRateHz, config_.testToneHz, config_.testToneAmplitude);
    return NodeState::CalibrateSecondaryPath;
}

bool NodeRuntime::useProcessor(IBlockProcessor* p) {
    processor_.store(p, std::memory_order_release);
    // Two blocks: the one in progress may have loaded the old pointer, so
    // the second is the first that certainly ran p.
    const uint32_t start = filterBlocks_.load(std::memory_order_relaxed);
    const uint32_t t0    = nowMs();
    while (filterBlocks_.load(std::memory_order_relaxed) - start < 2) {
        if (nowMs() - t0 > kFilterResponseTimeoutMs) {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    return true;
}

bool NodeRuntime::fadeTo(float gain) {
    gainTarget_.store(gain, std::memory_order_relaxed);
    const uint32_t t0 = nowMs();
    while (outputGain_.load(std::memory_order_relaxed) != gain) {
        if (nowMs() - t0 > kFadeTimeoutMs) {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return true;
}

void NodeRuntime::configureAlignment() {
    alignCfg_ = config_;
    for (std::size_t t = 0; t < config_.numTones; ++t) {
        alignCfg_.initialFreqHz[t] = startFreqHz_[t];
    }
    alignConfigPending_.store(true, std::memory_order_release);
}

bool NodeRuntime::waitForAlignmentIdle() {
    const uint32_t t0 = nowMs();
    while (alignConfigPending_.load(std::memory_order_acquire)) {
        if (nowMs() - t0 > kFilterResponseTimeoutMs) {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return true;
}

void NodeRuntime::applyPendingConfig() {
    if (!configChanged_.exchange(false, std::memory_order_relaxed)) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        config_ = pendingConfig_;
    }
    scheduler_ = CalibrationScheduler(config_.backstopIntervalMs, config_.cooldownMs);
    outputCeiling_.store(config_.outputCeiling, std::memory_order_relaxed);
    ESP_LOGI(kTag, "new settings applied: taps=%u mu=%g tones=%u",
             static_cast<unsigned>(config_.secondaryPathTaps),
             static_cast<double>(config_.normalizedStepSize),
             static_cast<unsigned>(config_.numTones));
}

NodeState NodeRuntime::doCalibrate() {
    // Settings changed with `set` take effect here, and this calibration
    // answers any `recal` typed before it.
    applyPendingConfig();
    recalRequested_.store(false, std::memory_order_relaxed);

    // Remember where the hum was. Calibration restarts the trackers, and
    // they should pick up from the last locked frequency rather than walk
    // back from the config's hint (at most a few Hz per second).
    for (std::size_t t = 0; t < config_.numTones; ++t) {
        startFreqHz_[t] = trackedLocked_[t].load(std::memory_order_relaxed)
                              ? trackedFreqHz_[t].load(std::memory_order_relaxed)
                              : config_.initialFreqHz[t];
    }

    // 1. Fade out whatever is playing, then switch to silence.
    // 2. Fade "up" on silence. The probe then plays at full gain from its
    //    first sample, so the speaker plays exactly what is recorded; a
    //    fade-in on the probe itself would skew the estimate.
    if (!fadeTo(0.0f) || !useProcessor(&silence_) || !fadeTo(1.0f)) {
        ESP_LOGE(kTag, "calibration: FilterTask not responding");
        faultCause_ = "filter task";
        return NodeState::Fault;
    }

    std::size_t samples =
        config_.probeDurationMs * static_cast<uint32_t>(kSampleRateHz) / 1000;
    if (samples > ProbeProcessor::kMaxSamples) {
        ESP_LOGW(kTag,
                 "probe_duration_ms %u is longer than the recorder holds; "
                 "using %u ms",
                 static_cast<unsigned>(config_.probeDurationMs),
                 static_cast<unsigned>(ProbeProcessor::kMaxSamples * 1000 /
                                       static_cast<uint32_t>(kSampleRateHz)));
        samples = ProbeProcessor::kMaxSamples;
    }

    // 3. Play the probe until the recording is full. probe_ isn't active
    //    yet, so begin() can't race FilterTask; the store publishes it.
    ESP_LOGI(
        kTag, "calibration: playing %u ms probe",
        static_cast<unsigned>(samples * 1000 / static_cast<uint32_t>(kSampleRateHz)));
    probeDone_.store(false, std::memory_order_relaxed);
    probe_.begin(samples);
    processor_.store(&probe_, std::memory_order_release);

    const uint32_t t0     = nowMs();
    const uint32_t giveUp = 2 * config_.probeDurationMs;
    bool finished         = false;
    while (!(finished = probeDone_.load(std::memory_order_acquire))) {
        if (nowMs() - t0 > giveUp) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    // Leave the output silent and faded down, like FAULT does, so RUN's
    // first sound fades in.
    (void)useProcessor(&silence_);
    gainTarget_.store(0.0f, std::memory_order_relaxed);
    if (!finished) {
        ESP_LOGE(kTag, "calibration: probe didn't finish in %u ms",
                 static_cast<unsigned>(giveUp));
        faultCause_ = "probe timeout";
        return NodeState::Fault;
    }

    // 4. Estimate the impulse response. FilterTask has stopped writing the
    //    recording (it is full), so reading it here is safe. AlignmentTask
    //    reads secondaryPath_ while it configures, so let it finish first.
    if (!waitForAlignmentIdle()) {
        ESP_LOGE(kTag, "calibration: AlignmentTask not responding");
        faultCause_ = "alignment task";
        return NodeState::Fault;
    }
    const auto& rec = probe_.recording();
    const std::size_t taps =
        std::min<std::size_t>(config_.secondaryPathTaps, kMaxSecondaryPathTaps);
    estimateImpulseResponse(rec.probe(), rec.mic(), rec.size(), secondaryPath_, taps);
    secondaryPathTaps_ = taps;

    const IrPeak peak = findPeak(secondaryPath_, taps);
    ESP_LOGI(kTag, "calibration: peak at tap %u, value %.4f, delay %.2f ms",
             static_cast<unsigned>(peak.index), peak.value,
             delayMs(peak.index, kSampleRateHz));

    // A response with no clear peak means the mic didn't hear the probe.
    // White-noise leakage alone puts the biggest of N taps only a few
    // times above their RMS; a real path stands well clear of that.
    double sumSquares = 0.0;
    for (std::size_t k = 0; k < taps; ++k) {
        sumSquares += static_cast<double>(secondaryPath_[k]) * secondaryPath_[k];
    }
    const float tapRms = static_cast<float>(std::sqrt(sumSquares / taps));
    if (std::fabs(peak.value) < kMinPeakToRms * tapRms) {
        ESP_LOGW(kTag,
                 "calibration: no clear peak -- is the speaker playing and "
                 "the mic connected?");
    }
    // The peak always sits a fixed delay in (about 160 samples of buffering
    // plus the acoustic path), so what matters is how much room is left after
    // it. A small speaker rings for a while at low hums; with fewer than 8 ms
    // of taps after the peak, the canceller works from a cut-off model.
    if (peak.index + kMinTailTaps > taps) {
        ESP_LOGW(kTag,
                 "calibration: peak at tap %u leaves under %u taps for the "
                 "speaker's ring; raise dsp.secondary_path_taps",
                 static_cast<unsigned>(peak.index),
                 static_cast<unsigned>(kMinTailTaps));
    }

    // One line per tap, for tools/ir_from_log.py.
    for (std::size_t k = 0; k < taps; ++k) {
        ESP_LOGI(kTag, "ir,%u,%.6f", static_cast<unsigned>(k), secondaryPath_[k]);
    }

    configureAlignment();
    scheduler_.markCalibrated(nowMs());
    return NodeState::Run;
}

NodeState NodeRuntime::doRun() {
    // M5: cancel the configured hum. Start from scratch every time: after
    // a fault the old weights may be the reason we stopped.
    canceller_.reset();
    canceller_.setSecondaryPath(secondaryPath_, secondaryPathTaps_);
    canceller_.setNumTones(config_.numTones);
    for (std::size_t t = 0; t < config_.numTones; ++t) {
        canceller_.setToneFrequency(t, startFreqHz_[t]);
    }
    canceller_.setNormalizedStepSize(config_.normalizedStepSize);

    diverged_.store(false, std::memory_order_relaxed);
    processor_.store(&fxlms_, std::memory_order_release);
    gainTarget_.store(1.0f, std::memory_order_relaxed);
    ESP_LOGI(kTag, "cancelling %u tone(s) from %.1f Hz, mu %.4f",
             static_cast<unsigned>(config_.numTones), startFreqHz_[0],
             config_.normalizedStepSize);

    // Check often: a diverging canceller gets loud within a fraction of a
    // second, and FAULT is what mutes it.
    for (;;) {
        if (diverged_.exchange(false, std::memory_order_relaxed)) {
            ESP_LOGE(kTag, "divergence detected");
            faultCause_ = "divergence";
            return NodeState::Fault;
        }
        // Asked for at the console: skips the cooldown, since a person
        // asked for it.
        if (recalRequested_.load(std::memory_order_relaxed)) {
            ESP_LOGI(kTag, "rechecking the speaker-to-mic path: requested");
            return NodeState::RecheckSecondaryPath;
        }
        const bool drift = alignNeedsCalibration_.load(std::memory_order_relaxed);
        if (scheduler_.shouldRecheck(nowMs(), drift)) {
            ESP_LOGI(
                kTag, "rechecking the speaker-to-mic path: %s",
                drift ? "the model no longer matches" : "backstop interval reached");
            return NodeState::RecheckSecondaryPath;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void NodeRuntime::audioIoLoop() {
    float mic[kBlockSamples];
    float out[kBlockSamples];

    for (;;) {
        const std::size_t n = audio_.readBlock(mic, kBlockSamples);
        if (n == 0) {
            // A failed read returns at once. Without this delay the task
            // would spin and starve everything below it on this core.
            vTaskDelay(1);
            continue;
        }
        for (std::size_t i = 0; i < n; ++i) {
            // A full ring means FilterTask is far behind. Drop the sample:
            // this task must never wait.
            (void)micRing_.push(mic[i]);
        }
        xTaskNotifyGive(filterTask_);

        // Normally the ring holds exactly the lead here. After an underrun
        // a late FilterTask catches up and leaves extra samples behind,
        // which would make every later sample reach the speaker later than
        // calibration measured. Skip the stale ones so the delay stays
        // fixed. FilterTask can't push while this runs (lower priority,
        // same core), so size() is exact.
        float stale = 0.0f;
        while (outRing_.size() > kOutputLeadSamples && outRing_.pop(stale)) {
        }

        uint32_t missing = 0;
        for (std::size_t i = 0; i < n; ++i) {
            if (!outRing_.pop(out[i])) {
                out[i] = 0.0f;
                ++missing;
            }
        }
        if (missing > 0) {
            underruns_.fetch_add(missing, std::memory_order_relaxed);
        }

        audio_.writeBlock(out, n);
        blocksProcessed_.fetch_add(1, std::memory_order_relaxed);
    }
}

void NodeRuntime::filterLoop() {
    float mic[kBlockSamples];
    float out[kBlockSamples];
    float micLevelDb                = -120.0f;
    float rampTarget                = 0.0f;  // the target ramp_ is heading for
    const IBlockProcessor* previous = nullptr;

    for (;;) {
        // Collect exactly one block. When the ring runs dry, sleep until
        // AudioIOTask's next notification. The timeout only matters if
        // audio stops altogether.
        std::size_t have = 0;
        while (have < kBlockSamples) {
            if (micRing_.pop(mic[have])) {
                ++have;
            } else {
                ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
            }
        }

        // Level meter: rises at once, falls back slowly. Telemetry only
        // looks once a second, so a plain per-block value would almost
        // always miss a clap.
        const float blockDb = amplitudeToDb(rms(mic, kBlockSamples));
        micLevelDb          = std::max(blockDb, micLevelDb - kMicMeterFallDbPerBlock);
        micDbfs_.store(micLevelDb, std::memory_order_relaxed);

        // Load the pointer once per block, so a swap never lands mid-block.
        IBlockProcessor* processor = processor_.load(std::memory_order_acquire);
        if (processor != previous && processor == &fxlms_) {
            divergence_.reset();  // a fresh start for a fresh canceller
        }
        previous = processor;
        processor->processBlock(mic, out, kBlockSamples);

        // A NaN or infinity must never reach the limiter (every comparison
        // with NaN is false, so it would pass straight through to the amp).
        // From the canceller it means it has blown up.
        bool nonFinite = false;
        for (std::size_t i = 0; i < kBlockSamples; ++i) {
            if (!std::isfinite(out[i])) {
                out[i]    = 0.0f;
                nonFinite = true;
            }
        }

        const float target = gainTarget_.load(std::memory_order_relaxed);
        if (target != rampTarget) {
            ramp_.setTarget(target);
            rampTarget = target;
        }
        const float ceiling = outputCeiling_.load(std::memory_order_relaxed);
        uint32_t clips      = 0;
        for (std::size_t i = 0; i < kBlockSamples; ++i) {
            bool clipped = false;
            out[i]       = limitOutput(ramp_.process(out[i]), ceiling, clipped);
            if (clipped) {
                ++clips;
            }
        }
        if (clips > 0) {
            clipEvents_.fetch_add(clips, std::memory_order_relaxed);
        }
        outputGain_.store(ramp_.gain(), std::memory_order_relaxed);
        if (processor == &probe_ && probe_.done()) {
            probeDone_.store(true, std::memory_order_release);
        }
        if (processor == &fxlms_) {
            // Follow the hum as it drifts (M6), but only on a frequency the
            // tracker trusts: locked, and not possibly locked onto our own
            // output. Takes effect from the next block.
            const bool trusted = !alignLowConfidence_.load(std::memory_order_relaxed);
            for (std::size_t t = 0; t < canceller_.numTones(); ++t) {
                if (trusted && trackedLocked_[t].load(std::memory_order_relaxed)) {
                    canceller_.setToneFrequency(
                        t, trackedFreqHz_[t].load(std::memory_order_relaxed));
                }
            }

            residualDbfs_.store(amplitudeToDb(std::sqrt(canceller_.residualPower())),
                                std::memory_order_relaxed);
            converged_.store(canceller_.converged(), std::memory_order_relaxed);
            weightNorm_.store(canceller_.weightNorm(), std::memory_order_relaxed);

            const float level = rms(mic, kBlockSamples);
            divergence_.update(level * level);
            if (divergence_.diverged() || nonFinite) {
                diverged_.store(true, std::memory_order_relaxed);
            }
        } else {
            converged_.store(false, std::memory_order_relaxed);
        }

        uint32_t dropped = 0;
        for (std::size_t i = 0; i < kBlockSamples; ++i) {
            // Can't fail: AudioIOTask keeps this ring near the lead.
            (void)outRing_.push(out[i]);
            if (!alignRing_.push({mic[i], out[i]})) {
                ++dropped;
            }
        }
        if (dropped > 0) {
            alignDrops_.fetch_add(dropped, std::memory_order_relaxed);
        }
        filterBlocks_.fetch_add(1, std::memory_order_relaxed);
    }
}

void NodeRuntime::alignmentLoop() {
    bool configured = false;  // nothing to track before the first calibration

    for (;;) {
        if (alignConfigPending_.load(std::memory_order_acquire)) {
            align_.configure(alignCfg_, secondaryPath_, secondaryPathTaps_);
            configured = true;
            alignConfigPending_.store(false, std::memory_order_release);
        }

        SamplePair pair{};
        uint32_t received = 0;
        while (alignRing_.pop(pair)) {
            if (configured) {
                align_.update(pair.mic, pair.synth);
            }
            humBuf_[humCount_++] = pair.mic;
            if (humCount_ == kHumSamples) {
                // 100 ms holds a whole number of cycles of 60 Hz (and 50 Hz),
                // so the measurement leaks little from nearby frequencies.
                const float amplitude = toneAmplitude(
                    humBuf_, kHumSamples,
                    trackedFreqHz_[0].load(std::memory_order_relaxed), kSampleRateHz);
                humDbfs_.store(amplitudeToDb(amplitude), std::memory_order_relaxed);
                humCount_ = 0;
            }
            ++received;
        }
        if (received > 0) {
            alignSamples_.fetch_add(received, std::memory_order_relaxed);
        }

        if (configured) {
            for (std::size_t t = 0; t < align_.numTones(); ++t) {
                trackedFreqHz_[t].store(align_.frequencyHz(t),
                                        std::memory_order_relaxed);
                trackedLocked_[t].store(align_.locked(t), std::memory_order_relaxed);
            }
            alignCorrelation_.store(align_.correlation(), std::memory_order_relaxed);
            alignLowConfidence_.store(align_.lowConfidence(),
                                      std::memory_order_relaxed);
            alignNeedsCalibration_.store(align_.needsCalibration(),
                                         std::memory_order_relaxed);
        }

        // 5 ms is 40 pairs; the ring holds 2047.
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void NodeRuntime::handleCommand(const char* line) {
    const Command cmd = parseCommand(line);
    char value[24];

    switch (cmd.verb) {
        case Verb::Get: {
            bool ok = false;
            {
                std::lock_guard<std::mutex> lock(configMutex_);
                ok = formatSetting(pendingConfig_, cmd.key, value, sizeof(value));
            }
            if (ok) {
                std::printf("%s = %s\n", cmd.key, value);
            } else {
                std::printf("unknown key %s\n", cmd.key);
            }
            break;
        }

        case Verb::Set: {
            bool ok = false;
            {
                std::lock_guard<std::mutex> lock(configMutex_);
                ok = applySetting(pendingConfig_, cmd.key, cmd.value);
            }
            if (ok) {
                configChanged_.store(true, std::memory_order_relaxed);
                // Settings take effect at the next calibration: `recal`
                // starts one now. The test tone and DMA block size are only
                // read at boot, so those need `save` and a reboot.
                std::printf(
                    "ok, applies at the next calibration (type recal to "
                    "start one now)\n");
            } else {
                std::printf("rejected: unknown key, or value out of range\n");
            }
            break;
        }

        case Verb::Save: {
            NodeConfig copy;
            {
                std::lock_guard<std::mutex> lock(configMutex_);
                copy = pendingConfig_;
            }
            if (store_.save(copy)) {
                // Writing flash pauses both cores for a moment. That can
                // shift the audio loop's delay, which calibration has to
                // match, so measure it again.
                recalRequested_.store(true, std::memory_order_relaxed);
                std::printf(
                    "saved; recalibrating, since the flash write paused "
                    "the audio\n");
            } else {
                std::printf("save failed, see the log above\n");
            }
            break;
        }

        case Verb::Recal:
            recalRequested_.store(true, std::memory_order_relaxed);
            std::printf("recalibrating\n");
            break;

        case Verb::Status: {
            // The same atomics telemetryLoop() prints, on demand.
            const NodeStatus st = status_.read();
            std::printf(
                "state=%s freq=%.2f lock=%d resid=%.1fdBFS mic=%.1fdBFS\n",
                toString(st.state), trackedFreqHz_[0].load(std::memory_order_relaxed),
                static_cast<int>(trackedLocked_[0].load(std::memory_order_relaxed)),
                residualDbfs_.load(std::memory_order_relaxed),
                micDbfs_.load(std::memory_order_relaxed));
            break;
        }

        case Verb::Ir:
            // The last calibration's impulse response, in the same format
            // as doCalibrate() logs it, for tools/ir_from_log.py.
            if (secondaryPathTaps_ == 0) {
                std::printf("no impulse response yet\n");
                break;
            }
            for (std::size_t k = 0; k < secondaryPathTaps_; ++k) {
                std::printf("ir,%u,%.6f\n", static_cast<unsigned>(k),
                            secondaryPath_[k]);
            }
            break;

        case Verb::Help:
            std::printf(
                "commands:\n"
                "  get <key>          show a setting, e.g. get "
                "dsp.normalized_step_size\n"
                "  set <key> <value>  change it (applies at the next "
                "calibration)\n"
                "  save               write the settings to flash\n"
                "  recal              calibrate again now\n"
                "  status             show the state, hum and levels now\n"
                "  ir                 print the last impulse response\n"
                "  help               this list\n"
                "keys are the node.json names, e.g. "
                "calibration.probe_duration_ms (see docs/config.md)\n");
            break;

        case Verb::Invalid:
            std::printf("unknown command, try help\n");
            break;
    }
}

void NodeRuntime::configLoop() {
    // Build up one line at a time. The console hands over characters as
    // they arrive (the monitor sends each key press), so a single read
    // can hold half a command.
    char line[96];
    std::size_t len  = 0;
    bool overflowing = false;

    for (;;) {
        const int c = std::fgetc(stdin);
        if (c == EOF) {
            // Nothing typed yet. stdin doesn't block here, so clear the
            // end-of-file flag and look again shortly.
            std::clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (c == '\r' || c == '\n') {
            if (overflowing) {
                std::printf("line too long, try help\n");
            } else if (len > 0) {
                line[len] = '\0';
                std::printf("> %s\n", line);  // the monitor doesn't echo
                handleCommand(line);
            }
            len         = 0;
            overflowing = false;
        } else if (len + 1 < sizeof(line)) {
            line[len++] = static_cast<char>(c);
        } else {
            overflowing = true;
        }
    }
}

void NodeRuntime::telemetryLoop() {
    for (uint32_t tick = 1;; ++tick) {
        NodeStatus s = status_.read();
        coordinator_.report(s);
        ESP_LOGI(
            "telemetry",
            "up=%u state=%s conv=%d resid=%.1fdBFS freq=%.2f lock=%d corr=%.2f low=%d "
            "recal=%d "
            "clip=%u blocks=%u filter=%u under=%u align=%u drop=%u mic=%.1fdBFS "
            "hum=%.1fdBFS wnorm=%.3f faults=%u",
            static_cast<unsigned>(esp_timer_get_time() / 1000000), toString(s.state),
            static_cast<int>(converged_.load(std::memory_order_relaxed)),
            residualDbfs_.load(std::memory_order_relaxed),
            trackedFreqHz_[0].load(std::memory_order_relaxed),
            static_cast<int>(trackedLocked_[0].load(std::memory_order_relaxed)),
            alignCorrelation_.load(std::memory_order_relaxed),
            static_cast<int>(alignLowConfidence_.load(std::memory_order_relaxed)),
            static_cast<int>(alignNeedsCalibration_.load(std::memory_order_relaxed)),
            static_cast<unsigned>(clipEvents_.load(std::memory_order_relaxed)),
            static_cast<unsigned>(blocksProcessed_.load(std::memory_order_relaxed)),
            static_cast<unsigned>(filterBlocks_.load(std::memory_order_relaxed)),
            static_cast<unsigned>(underruns_.load(std::memory_order_relaxed)),
            static_cast<unsigned>(alignSamples_.load(std::memory_order_relaxed)),
            static_cast<unsigned>(alignDrops_.load(std::memory_order_relaxed)),
            micDbfs_.load(std::memory_order_relaxed),
            humDbfs_.load(std::memory_order_relaxed),
            weightNorm_.load(std::memory_order_relaxed),
            static_cast<unsigned>(faults_.load(std::memory_order_relaxed)));
        // The line above shows tone 0 only. With harmonics, list them all
        // as frequency/lock.
        if (config_.numTones > 1) {
            char tones[96];
            std::size_t used = 0;
            for (std::size_t t = 0; t < config_.numTones && used < sizeof(tones); ++t) {
                const int n = std::snprintf(
                    tones + used, sizeof(tones) - used, "%s%.2f/%d", t == 0 ? "" : " ",
                    trackedFreqHz_[t].load(std::memory_order_relaxed),
                    static_cast<int>(
                        trackedLocked_[t].load(std::memory_order_relaxed)));
                if (n < 0) {
                    break;
                }
                used += static_cast<std::size_t>(n);
            }
            ESP_LOGI("telemetry", "tones: %s", tones);
        }
        if (tick % kStackReportEvery == 0) {
            // In ESP-IDF the high-water mark is in bytes. 0 means the task
            // never started. nullptr asks about this task itself.
            ESP_LOGI("telemetry",
                     "stack free: audio_io=%u filter=%u alignment=%u config=%u "
                     "telemetry=%u",
                     stackFree(audioIoTask_), stackFree(filterTask_),
                     stackFree(alignmentTask_), stackFree(configTask_),
                     static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
            // On one board this should always say 1.
            ESP_LOGI("telemetry", "coordinator: %u node(s)",
                     static_cast<unsigned>(coordinator_.nodeCount()));
        }
        vTaskDelay(pdMS_TO_TICKS(kTelemetryPeriodMs));
    }
}

void NodeRuntime::spawnTasks() {
    // TelemetryTask: 1Hz status over UART. Non-real-time, core 1.
    xTaskCreatePinnedToCore(
        [](void* arg) { static_cast<NodeRuntime*>(arg)->telemetryLoop(); }, "telemetry",
        4096, this, 2, nullptr, kNonRealtimeCore);

    // Start the speaker ring at its lead, so the first blocks don't
    // underrun while FilterTask gets going.
    for (std::size_t i = 0; i < kOutputLeadSamples; ++i) {
        (void)outRing_.push(0.0f);
    }

    // FilterTask: the DSP. Same core as AudioIOTask, one priority below,
    // so it runs in the gaps while AudioIOTask waits on I2S. Created first
    // because AudioIOTask notifies it.
    xTaskCreatePinnedToCore(
        [](void* arg) { static_cast<NodeRuntime*>(arg)->filterLoop(); }, "filter", 4096,
        this, configMAX_PRIORITIES - 3, &filterTask_, kRealtimeCore);

    // AudioIOTask: the only task that touches I2S. Real-time core, just
    // below the top priority (ESP-IDF's IPC tasks keep the very top).
    xTaskCreatePinnedToCore(
        [](void* arg) { static_cast<NodeRuntime*>(arg)->audioIoLoop(); }, "audio_io",
        4096, this, configMAX_PRIORITIES - 2, &audioIoTask_, kRealtimeCore);

    // AlignmentTask: frequency tracking and drift checks (M6). Core 1, so
    // it can fall behind without touching the audio path.
    xTaskCreatePinnedToCore(
        [](void* arg) { static_cast<NodeRuntime*>(arg)->alignmentLoop(); }, "alignment",
        4096, this, 3, &alignmentTask_, kNonRealtimeCore);

    // ConfigTask: serial commands. Lowest priority on core 1. A bigger
    // stack than the others, because `save` runs cJSON and LittleFS.
    xTaskCreatePinnedToCore(
        [](void* arg) { static_cast<NodeRuntime*>(arg)->configLoop(); }, "config", 6144,
        this, 1, &configTask_, kNonRealtimeCore);

    ESP_LOGI(kTag, "tasks spawned; type help for serial commands");
}

void NodeRuntime::start() {
    ESP_LOGI(kTag, "ANC node starting");

    for (;;) {
        NodeState next = state_;

        switch (state_) {
            case NodeState::Boot:
                next = doBoot();
                break;

            case NodeState::InitHal:
                next = doInitHal();
                if (next != NodeState::Fault) {
                    halReady_ = true;
                    spawnTasks();  // once HAL exists, tasks can run
                }
                break;

            case NodeState::CalibrateSecondaryPath:
            case NodeState::RecheckSecondaryPath:
                next = doCalibrate();
                break;

            case NodeState::Run:
                next = doRun();
                break;

            case NodeState::Fault:
                // Design doc 5.9: mute at once. Silence takes over at
                // FilterTask's next block (no fade: whatever was playing
                // may be the problem), and the gain drops to 0 so the next
                // sound fades in. Calibration and RUN turn it back on.
                processor_.store(&silence_, std::memory_order_release);
                gainTarget_.store(0.0f, std::memory_order_relaxed);
                faults_.fetch_add(1, std::memory_order_relaxed);
                ESP_LOGE(kTag, "fault (%s) -- output muted", faultCause_);
                next = NodeState::SafeMute;
                break;

            case NodeState::SafeMute:
                vTaskDelay(pdMS_TO_TICKS(config_.cooldownMs));
                // Without working I2S there is nothing to calibrate, so
                // try bringing the HAL up again instead.
                next =
                    halReady_ ? NodeState::CalibrateSecondaryPath : NodeState::InitHal;
                break;
        }

        transitionTo(next);
    }
}

}  // namespace anc
