#pragma once

#include <cstddef>

namespace anc {

// Finite impulse response filter: a weighted sum of the last N inputs,
//   y(n) = sum over k of coeff[k] * x(n - k).
// Used to model the speaker-to-mic ("secondary") path, see design doc
// 10.3. Fixed-size storage keeps it off the heap, so it is safe in the
// real-time path (design doc 10.1).
template <std::size_t MaxTaps>
class FirFilter {
public:
    // Copies n coefficients (clamped to MaxTaps). Clears the history, so
    // old samples are never weighted by the new coefficients.
    void setCoefficients(const float* coeffs, std::size_t n) {
        taps_ = n < MaxTaps ? n : MaxTaps;
        for (std::size_t k = 0; k < MaxTaps; ++k) {
            coeffs_[k] = k < taps_ ? coeffs[k] : 0.0f;
        }
        reset();
    }

    // Pushes one input sample and returns the filtered output.
    float process(float in) {
        if (taps_ == 0)
            return 0.0f;

        // history_ is circular: pos_ is where the newest sample goes, and
        // the sample k steps older sits k slots behind it.
        history_[pos_] = in;
        float sum      = 0.0f;
        std::size_t i  = pos_;
        for (std::size_t k = 0; k < taps_; ++k) {
            sum += coeffs_[k] * history_[i];
            i = (i == 0) ? taps_ - 1 : i - 1;
        }
        pos_ = (pos_ + 1 == taps_) ? 0 : pos_ + 1;
        return sum;
    }

    // Forgets past inputs but keeps the coefficients.
    void reset() {
        for (std::size_t k = 0; k < MaxTaps; ++k) {
            history_[k] = 0.0f;
        }
        pos_ = 0;
    }

    std::size_t taps() const { return taps_; }

private:
    float coeffs_[MaxTaps]{};
    float history_[MaxTaps]{};
    std::size_t taps_ = 0;
    std::size_t pos_  = 0;  // circular index into history_
};

}  // namespace anc
