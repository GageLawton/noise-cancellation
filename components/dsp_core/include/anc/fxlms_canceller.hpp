#pragma once

#include <cstddef>

#include "anc/fir_filter.hpp"
#include "anc/node_config.hpp"
#include "anc/ref_generator.hpp"
#include "anc/running_power.hpp"

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

    // Learning step: normalized LMS on the filtered reference. Call once
    // per sample, after nextOutput(), with the mic sample that heard it.
    //
    // The sign is MINUS: the mic hears hum + speaker, so the weights must
    // move to make the speaker oppose the hum. Design doc 6.1 writes "+"
    // because it treats the output as already inverted.
    void update(float mic) {
        const float squares = filteredSin_ * filteredSin_ + filteredCos_ * filteredCos_;
        // The average starts at 0, so on its own it would underestimate the
        // power for the first ~100 samples and take huge steps. The
        // instantaneous value is a floor until the average catches up.
        const float average = power_.add(squares);
        const float p       = average > squares ? average : squares;
        const float step    = mu_ / (kNlmsEpsilon + p) * mic;
        w_[0] -= step * filteredSin_;
        w_[1] -= step * filteredCos_;
    }

    float filteredSin() const { return filteredSin_; }
    float filteredCos() const { return filteredCos_; }
    float weight(std::size_t i) const { return w_[i]; }

private:
    RefGenerator ref_;
    FirFilter<kMaxPathTaps> pathSin_;
    FirFilter<kMaxPathTaps> pathCos_;
    RunningPower power_{0.01f};  // average of filteredSin^2 + filteredCos^2
    float mu_ = 0.0f;
    float w_[2]{};
    float filteredSin_ = 0.0f;
    float filteredCos_ = 0.0f;
};

}  // namespace anc
