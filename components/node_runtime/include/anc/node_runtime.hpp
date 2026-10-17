#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "anc/block_processor.hpp"
#include "anc/config_store.hpp"
#include "anc/i2s_audio_io.hpp"
#include "anc/node_config.hpp"
#include "anc/node_state.hpp"
#include "anc/node_status.hpp"
#include "anc/silence_processor.hpp"
#include "anc/spsc_ring_buffer.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace anc {

// Owns the node's lifecycle: config, state machine, and task setup.
//
// Audio runs in its own task (see audioIoLoop). The DSP stages are filled
// in milestone by milestone -- see design doc section 10.4.
class NodeRuntime {
public:
    // Runs Boot -> InitHal -> ... and spawns tasks. Does not return
    // under normal operation.
    void start();

    const NodeConfig& config() const { return config_; }
    NodeStatusStore& status() { return status_; }

private:
    void transitionTo(NodeState next);

    // State handlers. Each returns the next state to enter.
    NodeState doBoot();
    NodeState doInitHal();
    NodeState doCalibrate();
    NodeState doRun();

    void spawnTasks();

    // AudioIOTask body: read a mic block, hand it to FilterTask, play the
    // block FilterTask made earlier, repeat. The blocking I2S read paces
    // it at the sample rate.
    void audioIoLoop();

    // FilterTask body: wait for a full mic block, run the active
    // IBlockProcessor on it, queue the result for the speaker, repeat.
    void filterLoop();

    // Samples per audio block: 4 ms at 8 kHz, the same as the DMA block.
    static constexpr std::size_t kBlockSamples = 32;

    // How far FilterTask's output runs ahead of the speaker. Two blocks
    // means FilterTask can finish a block up to 4 ms late without a gap.
    // It is also part of the speaker-to-mic delay calibration measures,
    // so AudioIOTask holds it fixed (see audioIoLoop).
    static constexpr std::size_t kOutputLeadSamples = 2 * kBlockSamples;

    // Reads the factory-burned MAC and folds it into a 64-bit id.
    static uint64_t deriveNodeId();

    ConfigStore store_;
    NodeConfig config_{};
    NodeState state_ = NodeState::Boot;
    NodeStatusStore status_;
    uint64_t nodeId_ = 0;

    I2sAudioIo audio_;
    bool halReady_ = false;  // set once I2S is up and the tasks are running
    std::atomic<uint32_t> blocksProcessed_{0};  // audio blocks read and written

    // Mic samples, AudioIOTask -> FilterTask. 1023 usable slots, about
    // 128 ms of audio, so FilterTask can fall well behind before any drop.
    SpscRingBuffer<float, 1024> micRing_;
    SilenceProcessor silence_;
    // What FilterTask runs. Later milestones swap in the tone, the probe
    // and the canceller with a single store().
    std::atomic<IBlockProcessor*> processor_{&silence_};
    std::atomic<uint32_t> filterBlocks_{0};  // blocks FilterTask processed
    // Speaker samples, FilterTask -> AudioIOTask.
    SpscRingBuffer<float, 1024> outRing_;
    // Speaker samples AudioIOTask had to fill with 0 because FilterTask's
    // output wasn't ready.
    std::atomic<uint32_t> underruns_{0};
    // AudioIOTask wakes FilterTask with a task notification after each mic
    // block, so FilterTask starts at once instead of polling.
    TaskHandle_t filterTask_ = nullptr;
};

}  // namespace anc
