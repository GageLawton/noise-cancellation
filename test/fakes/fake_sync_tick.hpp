#pragma once

#include <cstdint>

#include "anc/sync_tick.hpp"

// Host-only sync tick for tests. Counts the ticks this node sends, and
// lets a test decide when a tick "arrives" from another node.
class FakeSyncTick final : public anc::ISyncTick {
public:
    void signal() override { ++signals_; }

    // True once for each earlier deliverTick() call. Otherwise false right
    // away; the timeout is ignored so tests never actually wait.
    bool wait(uint32_t /*timeoutMs*/) override {
        if (pending_ == 0)
            return false;
        --pending_;
        return true;
    }

    // Test helper: pretends a tick arrived from another node.
    void deliverTick() { ++pending_; }

    uint32_t signals() const { return signals_; }

private:
    uint32_t signals_ = 0;
    uint32_t pending_ = 0;  // delivered ticks not yet taken by wait()
};
