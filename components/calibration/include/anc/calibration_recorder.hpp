#pragma once

#include <cstddef>

namespace anc {

// Fixed-size store for the {probe played, mic heard} sample pairs
// collected during one calibration run. No allocation, so it can be
// filled from the real-time path.
template <std::size_t Capacity>
class CalibrationRecorder {
public:
    // Begins a new recording of `samples` pairs (clamped to Capacity).
    void start(std::size_t samples) {
        target_ = samples < Capacity ? samples : Capacity;
        count_  = 0;
    }

    // Stores one pair. Returns false, and stores nothing, once full.
    bool add(float probe, float mic) {
        if (full())
            return false;
        probe_[count_] = probe;
        mic_[count_]   = mic;
        ++count_;
        return true;
    }

    bool full() const { return count_ >= target_; }
    std::size_t size() const { return count_; }
    const float* probe() const { return probe_; }
    const float* mic() const { return mic_; }

private:
    float probe_[Capacity]{};
    float mic_[Capacity]{};
    std::size_t target_ = 0;
    std::size_t count_  = 0;
};

}  // namespace anc
