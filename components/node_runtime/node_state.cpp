#include "anc/node_state.hpp"

namespace anc {

const char* toString(NodeState s) {
    switch (s) {
        case NodeState::Boot:
            return "BOOT";
        case NodeState::InitHal:
            return "INIT_HAL";
        case NodeState::CalibrateSecondaryPath:
            return "CALIBRATE_SECONDARY_PATH";
        case NodeState::Run:
            return "RUN";
        case NodeState::RecheckSecondaryPath:
            return "RECHECK_SECONDARY_PATH";
        case NodeState::Fault:
            return "FAULT";
        case NodeState::SafeMute:
            return "SAFE_MUTE";
    }
    return "UNKNOWN";
}

}  // namespace anc
