#pragma once

#include <cmath>
#include <cstddef>

namespace anc {

// The biggest tap of a measured impulse response. index is the acoustic
// delay in samples; value keeps its sign.
struct IrPeak {
    std::size_t index;
    float value;
};

// Finds the tap with the largest absolute value. The first one wins a
// tie, so an all-zero response gives {0, 0}.
inline IrPeak findPeak(const float* h, std::size_t taps) {
    IrPeak peak{0, 0.0f};
    for (std::size_t k = 0; k < taps; ++k) {
        if (std::fabs(h[k]) > std::fabs(peak.value)) {
            peak = {k, h[k]};
        }
    }
    return peak;
}

// Converts a delay in samples to milliseconds.
inline float delayMs(std::size_t index, float sampleRateHz) {
    return 1000.0f * static_cast<float>(index) / sampleRateHz;
}

}  // namespace anc
