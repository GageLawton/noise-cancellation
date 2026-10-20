#pragma once

#include <cmath>
#include <cstddef>

#include "anc/rms.hpp"

namespace anc {

// Largest absolute sample value in n samples. Returns 0 for n == 0.
inline float peakAbs(const float* x, std::size_t n) {
    float peak = 0.0f;
    for (std::size_t i = 0; i < n; ++i) {
        const float magnitude = std::fabs(x[i]);
        if (magnitude > peak)
            peak = magnitude;
    }
    return peak;
}

// Crest factor in dB: how far the peak sits above the RMS level. A square
// wave is 0 dB and a sine is about 3.01 dB; a high value means short
// spikes that get close to clipping. Returns 0 when the RMS is 0 (silence
// or n == 0).
inline float crestFactorDb(const float* x, std::size_t n) {
    const float level = rms(x, n);
    if (level == 0.0f)
        return 0.0f;
    return 20.0f * std::log10(peakAbs(x, n) / level);
}

}  // namespace anc
