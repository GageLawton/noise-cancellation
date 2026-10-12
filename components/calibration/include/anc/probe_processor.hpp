#pragma once

#include <cstddef>
#include <cstdint>

#include "anc/block_processor.hpp"
#include "anc/calibration_recorder.hpp"
#include "anc/probe_generator.hpp"

namespace anc {

// Plays the white-noise calibration probe and records {probe, mic} pairs.
// Runs in the real-time FilterTask like any other block processor. Once
// the recording is full it outputs silence.
//
// The mic sample in a block responds to speaker samples played earlier,
// so pairing them by position is fine: the impulse response estimated
// from the recording captures that delay.
class ProbeProcessor final : public IBlockProcessor {
public:
    static constexpr std::size_t kMaxSamples = 4096;

    explicit ProbeProcessor(float amplitude = 0.5f, uint32_t seed = 1)
        : amplitude_(amplitude), seed_(seed), probe_(seed, amplitude) {}

    // Starts a new recording of `samples` pairs. The generator restarts
    // from the same seed, so every run plays the same probe.
    void begin(std::size_t samples) {
        probe_ = ProbeGenerator(seed_, amplitude_);
        recorder_.start(samples);
    }

    void processBlock(const float* mic, float* out, std::size_t n) override {
        for (std::size_t i = 0; i < n; ++i) {
            if (recorder_.full()) {
                out[i] = 0.0f;
            } else {
                out[i] = probe_.next();
                recorder_.add(out[i], mic[i]);
            }
        }
    }

    bool done() const { return recorder_.full(); }
    const CalibrationRecorder<kMaxSamples>& recording() const { return recorder_; }

private:
    float amplitude_;
    uint32_t seed_;
    ProbeGenerator probe_;
    CalibrationRecorder<kMaxSamples> recorder_;
};

}  // namespace anc
