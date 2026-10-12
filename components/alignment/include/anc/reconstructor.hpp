#pragma once

#include <cstddef>

#include "anc/fir_filter.hpp"

namespace anc {

// Estimates the room noise without our own speaker in it (design doc 6.0):
//   d_hat(n) = mic(n) - S * y(n)
// where S is the measured speaker-to-mic path and y is what we played.
class Reconstructor {
public:
    static constexpr std::size_t kMaxPathTaps = 1024;

    void setSecondaryPath(const float* coeffs, std::size_t n) {
        path_.setCoefficients(coeffs, n);
    }

    // Takes one mic sample and the speaker sample played at the same time,
    // and returns the estimated noise.
    float process(float mic, float synth) { return mic - path_.process(synth); }

private:
    FirFilter<kMaxPathTaps> path_;
};

}  // namespace anc
