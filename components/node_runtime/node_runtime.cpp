#include "anc/node_runtime.hpp"

#include <cstring>

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
    return NodeState::CalibrateSecondaryPath;
}

NodeState NodeRuntime::doCalibrate() {
    // M4: play probe, capture impulse response, compute FIR estimate.
    ESP_LOGI(kTag, "calibration (stub -- no probe until M4)");
    return NodeState::Run;
}

NodeState NodeRuntime::doRun() {
    // M5/M6 fill this in. For now the state machine parks here and the
    // telemetry task keeps proving the system is alive.
    ESP_LOGI(kTag, "running (stub -- no DSP until M5)");
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    return NodeState::Run;
}

void NodeRuntime::audioIoLoop() {
    float mic[kBlockSamples];
    float out[kBlockSamples] = {};  // silence until FilterTask exists

    for (;;) {
        const std::size_t n = audio_.readBlock(mic, kBlockSamples);
        if (n == 0) {
            // A failed read returns at once. Without this delay the task
            // would spin and starve everything below it on this core.
            vTaskDelay(1);
            continue;
        }
        audio_.writeBlock(out, n);
        blocksProcessed_.fetch_add(1, std::memory_order_relaxed);
    }
}

void NodeRuntime::spawnTasks() {
    // TelemetryTask: 1Hz status over UART. Non-real-time, core 1.
    xTaskCreatePinnedToCore(
        [](void* arg) {
            auto* self = static_cast<NodeRuntime*>(arg);
            for (;;) {
                NodeStatus s = self->status().read();
                ESP_LOGI("telemetry",
                         "state=%s conv=%d resid=%.6f freq=%.2f lock=%d clip=%u "
                         "blocks=%u",
                         toString(s.state), static_cast<int>(s.converged),
                         s.residualPower, s.numTones > 0 ? s.trackedFreqHz[0] : 0.0f,
                         s.numTones > 0 ? static_cast<int>(s.freqLocked[0]) : 0,
                         static_cast<unsigned>(s.clipEvents),
                         static_cast<unsigned>(
                             self->blocksProcessed_.load(std::memory_order_relaxed)));
                vTaskDelay(pdMS_TO_TICKS(kTelemetryPeriodMs));
            }
        },
        "telemetry", 4096, this, 2, nullptr, kNonRealtimeCore);

    // AudioIOTask: the only task that touches I2S. Real-time core, just
    // below the top priority (ESP-IDF's IPC tasks keep the very top).
    xTaskCreatePinnedToCore(
        [](void* arg) { static_cast<NodeRuntime*>(arg)->audioIoLoop(); }, "audio_io",
        4096, this, configMAX_PRIORITIES - 2, nullptr, kRealtimeCore);

    ESP_LOGI(kTag, "tasks spawned (filter/alignment pending M2)");
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
                ESP_LOGE(kTag, "fault -- muting output");
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
