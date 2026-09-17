#pragma once

#include <atomic>
#include <cstdint>

#include "anc/node_config.hpp"
#include "anc/node_state.hpp"

namespace anc {

// Snapshot of node state for telemetry and (later) the coordinator.
//
// Single-writer / multi-reader: only the owning task mutates it via
// NodeStatusStore. Readers get a plain copy.
struct NodeStatus {
    uint64_t  nodeId;                       // MAC-derived, globally unique
    NodeState state;

    bool      converged;
    float     residualPower;

    float     trackedFreqHz[kMaxTones];
    bool      freqLocked[kMaxTones];
    uint8_t   numTones;

    bool      lowConfidence;                // self-lock watch gate tripped
    uint32_t  clipEvents;                   // cumulative output clip count
    uint32_t  timestampMs;
};

// Lock-free-enough status holder: the writer publishes whole snapshots
// and readers take whole copies, so a reader never sees a half-updated
// struct with fields from two different moments.
//
// Uses a seqlock: the writer bumps a counter before and after writing,
// readers retry while the counter is odd or changed mid-read.
class NodeStatusStore {
public:
    void publish(const NodeStatus& s) {
        seq_.fetch_add(1, std::memory_order_acquire);  // now odd: write in progress
        status_ = s;
        seq_.fetch_add(1, std::memory_order_release);  // now even: settled
    }

    NodeStatus read() const {
        NodeStatus out;
        uint32_t before;
        do {
            before = seq_.load(std::memory_order_acquire);
            if (before & 1u) continue;      // writer mid-update, retry
            out = status_;
        } while (before != seq_.load(std::memory_order_acquire));
        return out;
    }

private:
    NodeStatus status_{};
    mutable std::atomic<uint32_t> seq_{0};
};

}  // namespace anc
