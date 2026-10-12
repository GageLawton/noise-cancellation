#pragma once

#include <cstddef>

namespace anc {

// Fades a gain in a straight line toward a target, so switching the
// output on or off doesn't click. Starts at gain 0.
class GainRamp {
public:
    explicit GainRamp(std::size_t rampSamples) : rampSamples_(rampSamples) {}

    // From now on, move from the current gain to `target` over rampSamples.
    void setTarget(float target) {
        target_    = target;
        remaining_ = rampSamples_;
        if (remaining_ == 0) {
            gain_ = target;
        } else {
            step_ = (target - gain_) / static_cast<float>(remaining_);
        }
    }

    // Returns x scaled by the current gain, then steps the gain one sample
    // toward the target, stopping exactly on it.
    float process(float x) {
        const float y = x * gain_;
        if (remaining_ > 0) {
            --remaining_;
            // Land exactly on the target at the end, whatever rounding the
            // steps picked up along the way.
            gain_ = (remaining_ == 0) ? target_ : gain_ + step_;
        }
        return y;
    }

    float gain() const { return gain_; }

private:
    std::size_t rampSamples_;
    float gain_            = 0.0f;
    float target_          = 0.0f;
    float step_            = 0.0f;
    std::size_t remaining_ = 0;  // samples left in the current ramp
};

}  // namespace anc
