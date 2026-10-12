#pragma once

#include <cstddef>

namespace anc {

// Turns one block of mic samples into one block of speaker samples.
// Both are floats in [-1, 1]. Called from the real-time FilterTask, so
// implementations must not allocate, lock or block.
class IBlockProcessor {
public:
    virtual ~IBlockProcessor()                                             = default;
    virtual void processBlock(const float* mic, float* out, std::size_t n) = 0;
};

}  // namespace anc
