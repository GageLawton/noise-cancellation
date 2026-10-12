#pragma once

#include <cstddef>

namespace anc {

// Spots a canceller going unstable (design doc 5.9): residual power above
// a hard limit, or growing block over block for too long. Once tripped it
// stays tripped until reset(), so the runtime can't miss it.
class DivergenceDetector {
public:
    DivergenceDetector(float hardLimit, std::size_t growthBlocks)
        : hardLimit_(hardLimit), growthBlocks_(growthBlocks) {}

    // Call once per block with that block's residual power.
    void update(float blockPower) {
        if (blockPower > hardLimit_) {
            diverged_ = true;
        }
        // "Growing" means more than 5% louder than the block before.
        if (havePrevious_ && blockPower > kGrowthFactor * previous_) {
            ++growthCount_;
        } else {
            growthCount_ = 0;
        }
        if (growthCount_ >= growthBlocks_) {
            diverged_ = true;
        }
        previous_     = blockPower;
        havePrevious_ = true;
    }

    bool diverged() const { return diverged_; }

    void reset() {
        diverged_     = false;
        havePrevious_ = false;
        growthCount_  = 0;
        previous_     = 0.0f;
    }

private:
    static constexpr float kGrowthFactor = 1.05f;

    float hardLimit_;
    std::size_t growthBlocks_;
    float previous_          = 0.0f;
    bool havePrevious_       = false;
    std::size_t growthCount_ = 0;
    bool diverged_           = false;
};

}  // namespace anc
