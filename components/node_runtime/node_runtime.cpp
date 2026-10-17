#include "anc/node_runtime.hpp"

#include <algorithm>
#include <cstring>

#include "anc/decibels.hpp"
#include "anc/output_limiter.hpp"
#include "anc/rms.hpp"
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
    }
    s.timestampMs = nowMs();
    status_.publish(s);

    return NodeState::InitHal;
}

NodeState NodeRuntime::doInitHal() {
    if (!audio_.init(defaultI2sSettings())) {
        ESP_LOGE(kTag, "I2S init failed, check the mic and amp wiring");
        return NodeState::Fault;
    }
    tone_.emplace(kSampleRateHz, config_.testToneHz, config_.testToneAmplitude);
    return NodeState::CalibrateSecondaryPath;
}

NodeState NodeRuntime::doCalibrate() {
    // M4: play probe, capture impulse response, compute FIR estimate.
    ESP_LOGI(kTag, "calibration (stub -- no probe until M4)");
    return NodeState::Run;
}

NodeState NodeRuntime::doRun() {
    // M3: prove the speaker path with a steady tone. M5 replaces this with
    // the canceller.
    processor_.store(&*tone_, std::memory_order_release);
    gainTarget_.store(1.0f, std::memory_order_relaxed);  // fade in, no click
    ESP_LOGI(kTag, "test tone %.1f Hz at %.2f", config_.testToneHz,
             config_.testToneAmplitude);
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    return NodeState::Run;
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
    float micLevelDb = -120.0f;
    float rampTarget = 0.0f;  // the target ramp_ is heading for

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
        processor->processBlock(mic, out, kBlockSamples);

        const float target = gainTarget_.load(std::memory_order_relaxed);
        if (target != rampTarget) {
            ramp_.setTarget(target);
            rampTarget = target;
        }
        uint32_t clips = 0;
        for (std::size_t i = 0; i < kBlockSamples; ++i) {
            bool clipped = false;
            out[i] = limitOutput(ramp_.process(out[i]), config_.outputCeiling, clipped);
            if (clipped) {
                ++clips;
            }
        }
        if (clips > 0) {
            clipEvents_.fetch_add(clips, std::memory_order_relaxed);
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
    for (;;) {
        SamplePair pair{};
        uint32_t received = 0;
        while (alignRing_.pop(pair)) {
            ++received;  // no processing yet; the engine arrives in M6
        }
        if (received > 0) {
            alignSamples_.fetch_add(received, std::memory_order_relaxed);
        }
        // 5 ms is 40 pairs; the ring holds 2047.
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void NodeRuntime::telemetryLoop() {
    for (;;) {
        NodeStatus s = status_.read();
        ESP_LOGI(
            "telemetry",
            "state=%s conv=%d resid=%.6f freq=%.2f lock=%d clip=%u "
            "blocks=%u filter=%u under=%u align=%u drop=%u mic=%.1fdBFS",
            toString(s.state), static_cast<int>(s.converged), s.residualPower,
            s.numTones > 0 ? s.trackedFreqHz[0] : 0.0f,
            s.numTones > 0 ? static_cast<int>(s.freqLocked[0]) : 0,
            static_cast<unsigned>(clipEvents_.load(std::memory_order_relaxed)),
            static_cast<unsigned>(blocksProcessed_.load(std::memory_order_relaxed)),
            static_cast<unsigned>(filterBlocks_.load(std::memory_order_relaxed)),
            static_cast<unsigned>(underruns_.load(std::memory_order_relaxed)),
            static_cast<unsigned>(alignSamples_.load(std::memory_order_relaxed)),
            static_cast<unsigned>(alignDrops_.load(std::memory_order_relaxed)),
            micDbfs_.load(std::memory_order_relaxed));
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
        4096, this, configMAX_PRIORITIES - 2, nullptr, kRealtimeCore);

    // AlignmentTask: frequency tracking and drift checks (M6). Core 1, so
    // it can fall behind without touching the audio path.
    xTaskCreatePinnedToCore(
        [](void* arg) { static_cast<NodeRuntime*>(arg)->alignmentLoop(); }, "alignment",
        4096, this, 3, nullptr, kNonRealtimeCore);

    ESP_LOGI(kTag, "tasks spawned");
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
                ESP_LOGE(kTag, "fault -- output muted");
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
