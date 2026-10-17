#pragma once

#include <cstdint>

namespace anc {

// A shared "tick" that keeps boards in step (M7, design doc section 4).
// On hardware it will be a GPIO wire between the nodes; on a laptop a
// fake stands in, so coordinator logic can be tested before any wiring
// exists.
class ISyncTick {
public:
    virtual ~ISyncTick() = default;

    // Sends a tick to the other nodes.
    virtual void signal() = 0;

    // Waits up to timeoutMs for a tick. Returns true if one arrived in time.
    virtual bool wait(uint32_t timeoutMs) = 0;
};

}  // namespace anc
