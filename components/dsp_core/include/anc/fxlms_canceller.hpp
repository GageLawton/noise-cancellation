#pragma once

#include <cstddef>

#include "anc/fir_filter.hpp"
#include "anc/ref_generator.hpp"

namespace anc {

// Cancels one hum tone with FxLMS (design doc 6.1). It keeps two weights,
// one for a sine and one for a cosine at the hum frequency, and outputs
//   y = w0 * sin + w1 * cos.
// Learning needs the "filtered reference": the same sine and cosine
// passed through the measured speaker-to-mic path.
class FxlmsCanceller {
public:
    static constexpr std::size_t kMaxPathTaps = 1024;

    explicit FxlmsCanceller(float sampleRateHz) : ref_(sampleRateHz) {}

    // Sets the secondary path estimate used for both filtered references.
    void setSecondaryPath(const float* coeffs, std::size_t n) {
        pathSin_.setCoefficients(coeffs, n);
        pathCos_.setCoefficients(coeffs, n);
    }

    void setToneFrequency(float freqHz) { ref_.setFrequency(freqHz); }
    void setNormalizedStepSize(float mu) { mu_ = mu; }

    // Advances the reference by one sample, updates the filtered
    // reference, and returns the speaker sample.
    float nextOutput() {
        float s = 0.0f;
        float c = 0.0f;
        ref_.next(s, c);
        filteredSin_ = pathSin_.process(s);
        filteredCos_ = pathCos_.process(c);
        return w_[0] * s + w_[1] * c;
    }

    // Learning step: comes in the next issue (#114).
    void update(float /*mic*/) {}

    float filteredSin() const { return filteredSin_; }
    float filteredCos() const { return filteredCos_; }
    float weight(std::size_t i) const { return w_[i]; }

private:
    RefGenerator ref_;
    FirFilter<kMaxPathTaps> pathSin_;
    FirFilter<kMaxPathTaps> pathCos_;
    float mu_ = 0.0f;
    float w_[2]{};
    float filteredSin_ = 0.0f;
    float filteredCos_ = 0.0f;
};

}  // namespace anc
