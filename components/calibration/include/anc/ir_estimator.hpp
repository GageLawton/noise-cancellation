#pragma once

#include <cstddef>

namespace anc {

// Estimates the impulse response h of the path between a white-noise
// probe x and the recording e it produced. For white noise the
// cross-correlation is (almost exactly) the impulse response:
//   h[k] ~= sum over n of e[n] * x[n - k]  /  sum over n of x[n]^2
// Fills h[0..taps-1]. Uses only n >= k, so it never reads before the
// start of the recording. A silent probe gives all-zero taps.
inline void estimateImpulseResponse(const float* probe, const float* recorded,
                                    std::size_t n, float* h, std::size_t taps) {
    double energy = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        energy += static_cast<double>(probe[i]) * probe[i];
    }

    for (std::size_t k = 0; k < taps; ++k) {
        double sum = 0.0;
        for (std::size_t i = k; i < n; ++i) {
            sum += static_cast<double>(recorded[i]) * probe[i - k];
        }
        h[k] = energy > 0.0 ? static_cast<float>(sum / energy) : 0.0f;
    }
}

}  // namespace anc
