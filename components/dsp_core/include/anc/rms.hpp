#pragma once

#include <cmath>
#include <cstddef>

namespace anc {

// Root mean square of n samples. Returns 0 for n == 0.
inline float rms(const float* x, std::size_t n) {
    if (n == 0)
        return 0.0f;

    double sum = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const double sample = x[i];
        sum += sample * sample;
    }
    return static_cast<float>(std::sqrt(sum / static_cast<double>(n)));
}

}  // namespace anc
