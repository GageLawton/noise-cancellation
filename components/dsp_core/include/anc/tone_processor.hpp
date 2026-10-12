#pragma once

#include "anc/block_processor.hpp"
#include "anc/ref_generator.hpp"

namespace anc {

// Ignores the mic and fills the output with a steady sine. Milestone M3
// uses it to prove the speaker path with a fixed test tone.
class ToneProcessor final : public IBlockProcessor {
public:
    ToneProcessor(float sampleRateHz, float freqHz, float amplitude)
        : gen_(sampleRateHz), amplitude_(amplitude) {
        gen_.setFrequency(freqHz);
    }

    void processBlock(const float*, float* out, std::size_t n) override {
        float s = 0.0f;
        float c = 0.0f;
        for (std::size_t i = 0; i < n; ++i) {
            gen_.next(s, c);  // The cosine is not needed here.
            out[i] = amplitude_ * s;
        }
    }

private:
    RefGenerator gen_;
    float amplitude_;
};

}  // namespace anc
