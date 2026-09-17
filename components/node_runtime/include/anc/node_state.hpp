#pragma once

namespace anc {

// See design doc section 5.3.
//
//   Boot -> InitHal -> CalibrateSecondaryPath -> Run
//                            ^                    |
//                            |                    v
//                            +-- Fault <-- RecheckSecondaryPath
//                                 |
//                             SafeMute
enum class NodeState {
    Boot,                    // power-on, load config
    InitHal,                 // bring up I2S in/out, GPIO sync
    CalibrateSecondaryPath,  // first-boot calibration (no SecondaryPath yet)
    Run,                     // adaptive filtering active
    RecheckSecondaryPath,    // triggered recalibration, brief muted window
    Fault,                   // divergence or clipping; output muted
    SafeMute,                // held silent pending reset or cooldown
};

const char* toString(NodeState s);

}  // namespace anc
