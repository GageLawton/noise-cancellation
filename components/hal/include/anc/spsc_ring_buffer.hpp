#pragma once

#include <atomic>
#include <cstddef>

namespace anc {

// Lock-free single-producer / single-consumer ring buffer.
//
// One writer thread, one reader thread, no mutexes -- push() and pop()
// never block, which is what lets FilterTask (core 0, real-time) hand
// samples to AlignmentTask (core 1) without ever stalling on it.
//
// On a full buffer push() returns false and the caller drops the sample.
// That is the intended behavior for the FilterTask -> AlignmentTask path:
// alignment tolerates an occasional dropped sample, a missed audio
// deadline is not tolerable.
//
// Capacity must be a power of two so the index wrap is a mask, not a
// modulo. One slot is always left empty to distinguish full from empty,
// so usable capacity is Capacity - 1.
template <typename T, std::size_t Capacity>
class SpscRingBuffer {
    static_assert(Capacity >= 2, "capacity must be at least 2");
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "capacity must be a power of two");

public:
    // Producer side only.
    bool push(const T& value) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t next = (head + 1) & kMask;

        // acquire: don't let the slot write float above the tail read
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;  // full
        }

        buffer_[head] = value;
        // release: the slot write must be visible before the new head
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Consumer side only.
    bool pop(T& out) {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire)) {
            return false;  // empty
        }

        out = buffer_[tail];
        tail_.store((tail + 1) & kMask, std::memory_order_release);
        return true;
    }

    // Approximate -- safe to call from either side, but the value may be
    // stale by the time it is used. For telemetry, not for control flow.
    std::size_t size() const {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail) & kMask;
    }

    bool empty() const {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    static constexpr std::size_t capacity() { return Capacity - 1; }

private:
    static constexpr std::size_t kMask = Capacity - 1;

    T buffer_[Capacity]{};
    std::atomic<std::size_t> head_{0};  // written by producer
    std::atomic<std::size_t> tail_{0};  // written by consumer
};

}  // namespace anc
