#pragma once

#include "anc/block_processor.hpp"

namespace anc {

// Copies the mic straight to the output, scaled by a fixed gain. A quick
// bench check that the mic and speaker are both alive; keep the gain low
// to avoid feedback squeal.
class PassthroughProcessor final : public IBlockProcessor {
public:
    explicit PassthroughProcessor(float gain) : gain_(gain) {}

    void processBlock(const float* mic, float* out, std::size_t n) override {
        for (std::size_t i = 0; i < n; ++i) {
            out[i] = gain_ * mic[i];
        }
    }

private:
    float gain_;
};

}  // namespace anc
