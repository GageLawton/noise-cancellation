#pragma once

#include "anc/block_processor.hpp"

namespace anc {

// Output silence without inspecting microphone samples or allocating memory.
class SilenceProcessor final : public IBlockProcessor {
public:
    void processBlock(const float*, float* out, std::size_t n) override {
        for (std::size_t i = 0; i < n; ++i) {
            out[i] = 0.0f;
        }
    }
};

}  // namespace anc
