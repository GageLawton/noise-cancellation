#pragma once

#include <cstdint>

#include "anc/sync_message.hpp"

namespace anc {

// Checks incoming SyncMessages for gaps and late arrivals (M7). A gap in
// sequenceNumber means messages were lost; a number that isn't newer than
// the last one accepted is a repeat or a stale message and is dropped.
//
// Only one sender is tracked for now: nodeId is ignored. Sequence
// wraparound at 2^32 is ignored too; at one message a second that is
// over a century away.
class SyncReceiver {
public:
    // Returns false for a stale or repeated message. Otherwise records it,
    // counts any skipped sequence numbers, and returns true.
    bool accept(const SyncMessage& m) {
        // A sender counts from 1 again after it reboots, so sequence 1 starts
        // a new run instead of being dropped as stale forever.
        if (accepted_ > 0 && m.sequenceNumber == 1)
            ++restarts_;
        if (accepted_ > 0 && m.sequenceNumber != 1) {
            if (m.sequenceNumber <= lastSequence_)
                return false;
            missed_ += m.sequenceNumber - lastSequence_ - 1;
        }
        lastSequence_ = m.sequenceNumber;
        ++accepted_;
        return true;
    }

    // Total sequence numbers skipped so far (accepting 1 then 4 adds 2).
    uint32_t missed() const { return missed_; }

    uint32_t accepted() const { return accepted_; }

    // How many times the sender started again from 1 after a reboot.
    uint32_t restarts() const { return restarts_; }

private:
    uint32_t lastSequence_ = 0;
    uint32_t missed_       = 0;
    uint32_t restarts_     = 0;
    uint32_t accepted_     = 0;
};

}  // namespace anc
