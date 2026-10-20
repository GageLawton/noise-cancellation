#pragma once

#include <cmath>
#include <cstddef>

namespace anc {

// Peak amplitude of the freqHz component of n samples, using the Goertzel
// algorithm: one frequency at a time, without a full FFT. A sine of
// amplitude 0.5 at freqHz gives about 0.5. Returns 0 for n == 0.
//
// Most accurate when n holds a whole number of cycles of freqHz (for
// example 800 samples of 60 Hz at 8000 Hz); otherwise nearby frequencies
// leak in a little.
inline float toneAmplitude(const float* x, std::size_t n, float freqHz,
                           float sampleRateHz) {
    if (n == 0)
        return 0.0f;

    constexpr double kTwoPi = 6.283185307179586;
    const double w          = kTwoPi * freqHz / sampleRateHz;
    const double c          = 2.0 * std::cos(w);

    // Run a resonator tuned to w over the block; s1 and s2 are its last
    // two outputs.
    double s1 = 0.0;
    double s2 = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const double s0 = x[i] + c * s1 - s2;
        s2              = s1;
        s1              = s0;
    }

    const double power = s1 * s1 + s2 * s2 - c * s1 * s2;
    return static_cast<float>(2.0 * std::sqrt(std::fmax(power, 0.0)) /
                              static_cast<double>(n));
}

}  // namespace anc
