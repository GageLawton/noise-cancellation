#pragma once

#include <cstddef>

#include "anc/fir_filter.hpp"

namespace anc {

// Doubles the sample rate, for example 8 kHz up to 16 kHz.
//
// Why this exists: the partner to Decimator2x. If the bring-up test in
// issue #52 shows I2S has to run at 16 kHz, the 8 kHz speaker signal must
// be doubled in rate before it is sent to the amp.
//
// It inserts a zero after every input sample, then low-pass filters to
// smooth the gaps. Each input is pushed in as 2 * x to make up for the
// level the inserted zeros take away.
class Upsampler2x {
public:
    static constexpr std::size_t kTaps = 15;

    Upsampler2x() {
        // Same windowed-sinc half-band low-pass as Decimator2x.
        static constexpr float kCoeffs[kTaps] = {
            -0.0017f, 0.0f, 0.0134f,  0.0f, -0.0617f, 0.0f, 0.3002f, 0.5f,
            0.3002f,  0.0f, -0.0617f, 0.0f, 0.0134f,  0.0f, -0.0017f};
        filter_.setCoefficients(kCoeffs, kTaps);
    }

    // Writes 2 * n samples to out and returns 2 * n. The filter history
    // carries over between calls, so blocks join up seamlessly.
    std::size_t process(const float* in, std::size_t n, float* out) {
        for (std::size_t i = 0; i < n; ++i) {
            out[2 * i]     = filter_.process(2.0f * in[i]);
            out[2 * i + 1] = filter_.process(0.0f);
        }
        return 2 * n;
    }

    void reset() { filter_.reset(); }

private:
    FirFilter<kTaps> filter_;
};

}  // namespace anc
