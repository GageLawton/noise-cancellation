#pragma once

#include <cstddef>

#include "anc/fir_filter.hpp"
#include "anc/node_config.hpp"
#include "anc/ref_generator.hpp"
#include "anc/running_power.hpp"

namespace anc {

// Cancels up to kMaxTones hum tones with FxLMS (design doc 6.1). For each
// tone it keeps two weights, one for a sine and one for a cosine at that
// tone's frequency; the output is the sum over tones of
//   y_t = w[2t] * sin_t + w[2t+1] * cos_t.
// Learning needs the "filtered reference": each sine and cosine passed
// through the measured speaker-to-mic path.
class FxlmsCanceller {
public:
    static constexpr std::size_t kMaxPathTaps = 1024;
    static constexpr std::size_t kTones       = kMaxTones;

    explicit FxlmsCanceller(float sampleRateHz)
        : ref_{RefGenerator(sampleRateHz), RefGenerator(sampleRateHz),
               RefGenerator(sampleRateHz), RefGenerator(sampleRateHz)} {
        static_assert(kTones == 4, "update the ref_ initializer to match kMaxTones");
    }

    // Sets the secondary path estimate used for every filtered reference.
    void setSecondaryPath(const float* coeffs, std::size_t n) {
        for (std::size_t t = 0; t < kTones; ++t) {
            tones_[t].pathSin.setCoefficients(coeffs, n);
            tones_[t].pathCos.setCoefficients(coeffs, n);
        }
    }

    // Number of tones in use, clamped to [1, kMaxTones].
    void setNumTones(std::size_t n) {
        numTones_ = n < 1 ? 1 : (n > kTones ? kTones : n);
    }
    std::size_t numTones() const { return numTones_; }

    // Out-of-range tone indices are ignored.
    void setToneFrequency(std::size_t toneIdx, float freqHz) {
        if (toneIdx < kTones) {
            ref_[toneIdx].setFrequency(freqHz);
        }
    }

    void setNormalizedStepSize(float mu) { mu_ = mu; }

    // Advances every active reference by one sample, updates the filtered
    // references, and returns the speaker sample.
    float nextOutput() {
        float y = 0.0f;
        for (std::size_t t = 0; t < numTones_; ++t) {
            float s = 0.0f;
            float c = 0.0f;
            ref_[t].next(s, c);
            Tone& tone       = tones_[t];
            tone.filteredSin = tone.pathSin.process(s);
            tone.filteredCos = tone.pathCos.process(c);
            y += w_[2 * t] * s + w_[2 * t + 1] * c;
        }
        return y;
    }

    // Learning step: normalized LMS on the filtered references. Call once
    // per sample, after nextOutput(), with the mic sample that heard it.
    //
    // The sign is MINUS: the mic hears hum + speaker, so the weights must
    // move to make the speaker oppose the hum. Design doc 6.1 writes "+"
    // because it treats the output as already inverted.
    void update(float mic) {
        float squares = 0.0f;
        for (std::size_t t = 0; t < numTones_; ++t) {
            squares += tones_[t].filteredSin * tones_[t].filteredSin +
                       tones_[t].filteredCos * tones_[t].filteredCos;
        }
        // The average starts at 0, so on its own it would underestimate the
        // power for the first ~100 samples and take huge steps. The
        // instantaneous value is a floor until the average catches up.
        const float average = power_.add(squares);
        const float p       = average > squares ? average : squares;
        const float step    = mu_ / (kNlmsEpsilon + p) * mic;
        for (std::size_t t = 0; t < numTones_; ++t) {
            w_[2 * t] -= step * tones_[t].filteredSin;
            w_[2 * t + 1] -= step * tones_[t].filteredCos;
        }
    }

    float filteredSin(std::size_t toneIdx = 0) const {
        return tones_[toneIdx].filteredSin;
    }
    float filteredCos(std::size_t toneIdx = 0) const {
        return tones_[toneIdx].filteredCos;
    }
    // Weight i of 2 * kMaxTones: tone i / 2, sine if i is even, cosine if odd.
    float weight(std::size_t i) const { return w_[i]; }

private:
    struct Tone {
        FirFilter<kMaxPathTaps> pathSin;
        FirFilter<kMaxPathTaps> pathCos;
        float filteredSin = 0.0f;
        float filteredCos = 0.0f;
    };

    RefGenerator ref_[kTones];
    Tone tones_[kTones];
    // One shared average of every filtered sin^2 and cos^2.
    RunningPower power_{0.01f};
    std::size_t numTones_ = 1;
    float mu_             = 0.0f;
    float w_[2 * kTones]{};
};

}  // namespace anc
