#pragma once

#include <cstdint>

namespace anc {

// Repeatable white noise for calibration: the same seed always gives the
// same sequence. Uses a 32-bit xorshift generator, so it needs no library
// calls and no allocation.
class ProbeGenerator {
public:
    // xorshift is stuck at 0 forever, so a zero seed is replaced by 1.
    ProbeGenerator(uint32_t seed, float amplitude)
        : state_(seed == 0 ? 1u : seed), amplitude_(amplitude) {}

    // Returns the next sample, uniform in [-amplitude, amplitude).
    float next() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        // The top 24 bits fit a float exactly, so the result is in
        // [0, 1) without rounding up to 1. Then shift to [-1, 1).
        const float unit = static_cast<float>(state_ >> 8) * (1.0f / 16777216.0f);
        return (2.0f * unit - 1.0f) * amplitude_;
    }

private:
    uint32_t state_;
    float amplitude_;
};

}  // namespace anc
