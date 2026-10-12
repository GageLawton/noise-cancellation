#pragma once

#include <cstddef>
#include <cstdint>

#include "anc/node_status.hpp"

namespace anc {

// Collects the status of every node (design doc 5.1). Callers only use
// this interface, so it stays the same when more nodes arrive.
class ICoordinator {
public:
    virtual ~ICoordinator() = default;

    // Records the latest status from one node.
    virtual void report(const NodeStatus& s) = 0;

    // Copies the latest status of nodeId into out. Returns false, and
    // leaves out unchanged, if that node has never reported.
    virtual bool latest(uint64_t nodeId, NodeStatus& out) const = 0;

    // How many different nodes have reported.
    virtual std::size_t nodeCount() const = 0;
};

// Keeps the latest status per node, up to kMaxNodes. Single-threaded.
//
// Version 1 runs on one node, so this "local" coordinator just remembers
// what it hears. A report from a node already seen replaces its entry.
// Once all kMaxNodes slots are full, reports from new nodes are ignored.
class LocalCoordinator final : public ICoordinator {
public:
    static constexpr std::size_t kMaxNodes = 8;

    void report(const NodeStatus& s) override {
        NodeStatus* slot = find(s.nodeId);
        if (slot == nullptr) {
            if (count_ == kMaxNodes)
                return;  // full: ignore the new node
            slot = &nodes_[count_++];
        }
        *slot = s;
    }

    bool latest(uint64_t nodeId, NodeStatus& out) const override {
        for (std::size_t i = 0; i < count_; ++i) {
            if (nodes_[i].nodeId == nodeId) {
                out = nodes_[i];
                return true;
            }
        }
        return false;
    }

    std::size_t nodeCount() const override { return count_; }

private:
    NodeStatus* find(uint64_t nodeId) {
        for (std::size_t i = 0; i < count_; ++i) {
            if (nodes_[i].nodeId == nodeId)
                return &nodes_[i];
        }
        return nullptr;
    }

    NodeStatus nodes_[kMaxNodes]{};
    std::size_t count_ = 0;  // slots in use, filled in order
};

}  // namespace anc
