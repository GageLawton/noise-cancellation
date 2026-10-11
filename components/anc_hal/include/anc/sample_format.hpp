#pragma once

#include <cstdint>

namespace anc {

// INMP441: 24-bit sample, left-aligned in a 32-bit I2S slot.
inline float micSampleToFloat(int32_t raw) {
    return static_cast<float>(raw >> 8) / 8388608.0f;  // 2^23
}

}  // namespace anc
