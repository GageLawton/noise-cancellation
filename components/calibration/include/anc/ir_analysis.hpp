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

// The smallest n such that the first n taps hold at least fraction of
// the total energy (sum of squares): where the response has died away,
// for sizing secondary_path_taps. An all-zero response gives 0.
inline std::size_t tapsForEnergy(const float* h, std::size_t taps, float fraction) {
    double total = 0.0;
    for (std::size_t k = 0; k < taps; ++k) {
        total += static_cast<double>(h[k]) * h[k];
    }
    if (total == 0.0) {
        return 0;
    }
    // Same summation order as total, so fraction 1.0 stops at the last
    // non-zero tap rather than overshooting on rounding.
    const double target = fraction * total;
    double sum          = 0.0;
    std::size_t n       = 0;
    while (n < taps && sum < target) {
        sum += static_cast<double>(h[n]) * h[n];
        ++n;
    }
    return n;
}

}  // namespace anc
