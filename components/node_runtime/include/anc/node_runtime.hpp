#pragma once

#include <cstdint>

#include "anc/config_store.hpp"
#include "anc/node_config.hpp"
#include "anc/node_state.hpp"
#include "anc/node_status.hpp"

namespace anc {

// Owns the node's lifecycle: config, state machine, and task setup.
//
// M1 scope: boot through config load and task spawn. Audio and DSP
// tasks are stubs until M2/M3 -- see design doc section 10.4.
class NodeRuntime {
public:
    // Runs Boot -> InitHal -> ... and spawns tasks. Does not return
    // under normal operation.
    void start();

    const NodeConfig& config() const { return config_; }
    NodeStatusStore&  status()       { return status_; }

private:
    void transitionTo(NodeState next);

    // State handlers. Each returns the next state to enter.
    NodeState doBoot();
    NodeState doInitHal();
    NodeState doCalibrate();
    NodeState doRun();

    void spawnTasks();

    // Reads the factory-burned MAC and folds it into a 64-bit id.
    static uint64_t deriveNodeId();

    ConfigStore     store_;
    NodeConfig      config_{};
    NodeState       state_ = NodeState::Boot;
    NodeStatusStore status_;
    uint64_t        nodeId_ = 0;
};

}  // namespace anc
