#pragma once

#include <cstddef>

#include "anc/block_processor.hpp"
#include "anc/fxlms_canceller.hpp"

namespace anc {

// Runs an FxlmsCanceller as a block processor, so FilterTask can run it
// like the tone or the probe. The canceller is owned elsewhere (it is
// large, and the runtime configures it), so this only holds a reference.
class FxlmsProcessor final : public IBlockProcessor {
public:
    explicit FxlmsProcessor(FxlmsCanceller& canceller) : canceller_(canceller) {}

    void processBlock(const float* mic, float* out, std::size_t n) override {
        for (std::size_t i = 0; i < n; ++i) {
            out[i] = canceller_.nextOutput();
            canceller_.update(mic[i]);
        }
    }

private:
    FxlmsCanceller& canceller_;
};

}  // namespace anc
