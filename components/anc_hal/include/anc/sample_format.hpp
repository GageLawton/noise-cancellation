#pragma once

#include <cmath>
#include <cstdint>

namespace anc {

// INMP441: 24-bit sample, left-aligned in a 32-bit I2S slot.
inline float micSampleToFloat(int32_t raw) {
    return static_cast<float>(raw >> 8) / 8388608.0f;  // 2^23
}

// Converts a finite normalized sample to signed 16-bit speaker PCM.
// Clamp before rounding so out-of-range samples cannot wrap in the cast.
inline int16_t floatToSpeakerSample(float x) {
    if (x > 1.0f)
        x = 1.0f;
    if (x < -1.0f)
        x = -1.0f;
    return static_cast<int16_t>(std::lround(x * 32767.0f));
}

}  // namespace anc
