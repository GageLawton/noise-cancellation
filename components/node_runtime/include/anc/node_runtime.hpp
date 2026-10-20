#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "anc/config_store.hpp"
#include "anc/i2s_audio_io.hpp"
#include "anc/node_config.hpp"
#include "anc/node_state.hpp"
#include "anc/node_status.hpp"

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

    // AudioIOTask body: read a mic block, write a speaker block, repeat.
    // The blocking I2S read paces it at the sample rate.
    void audioIoLoop();

    // Samples per audio block: 4 ms at 8 kHz, the same as the DMA block.
    static constexpr std::size_t kBlockSamples = 32;

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
};

}  // namespace anc
