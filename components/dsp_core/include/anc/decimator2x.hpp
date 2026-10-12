#pragma once

#include <cstddef>

#include "anc/fir_filter.hpp"

namespace anc {

// Halves the sample rate, for example 16 kHz down to 8 kHz.
//
// Why this exists: some I2S MEMS mics (like the INMP441) need a faster
// clock than 8 kHz gives. If the bring-up test in issue #52 shows the
// mic misbehaves at 8 kHz, I2S runs at 16 kHz and this brings the mic
// signal down to the 8 kHz the DSP uses.
//
// Simply dropping every second sample would let tones above the new
// 4 kHz limit fold back down as fake tones ("aliasing"), so every input
// goes through a 15-tap low-pass filter first.
class Decimator2x {
public:
    static constexpr std::size_t kTaps = 15;

    Decimator2x() {
        // Windowed-sinc half-band low-pass. The taps sum to 1, so a slow
        // signal comes out at the same level it went in.
        static constexpr float kCoeffs[kTaps] = {
            -0.0017f, 0.0f, 0.0134f,  0.0f, -0.0617f, 0.0f, 0.3002f, 0.5f,
            0.3002f,  0.0f, -0.0617f, 0.0f, 0.0134f,  0.0f, -0.0017f};
        filter_.setCoefficients(kCoeffs, kTaps);
    }

    // Filters all n input samples and writes every second filtered one to
    // out, which needs room for (n + 1) / 2 samples. Returns how many were
    // written: n / 2 for even n. The filter history and which sample is
    // next to keep carry over between calls, so blocks of any size join
    // up seamlessly.
    std::size_t process(const float* in, std::size_t n, float* out) {
        std::size_t written = 0;
        for (std::size_t i = 0; i < n; ++i) {
            const float y = filter_.process(in[i]);
            if (keepNext_) {
                out[written++] = y;
            }
            keepNext_ = !keepNext_;
        }
        return written;
    }

    void reset() {
        filter_.reset();
        keepNext_ = false;
    }

private:
    FirFilter<kTaps> filter_;
    bool keepNext_ = false;  // true when the next filtered sample is kept
};

}  // namespace anc
