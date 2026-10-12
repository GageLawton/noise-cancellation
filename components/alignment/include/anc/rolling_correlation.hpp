#pragma once

#include <cmath>

namespace anc {

// Normalized correlation between two signals, averaged exponentially over
// roughly 1 / lambda samples. Near 0 means unrelated, near +1 or -1 means
// the same signal (or its negative). Alignment uses it to spot a copy of
// our own output left in the estimated noise.
class RollingCorrelation {
public:
    explicit RollingCorrelation(float lambda) : lambda_(lambda) {}

    void update(float a, float b) {
        ab_ += lambda_ * (a * b - ab_);
        aa_ += lambda_ * (a * a - aa_);
        bb_ += lambda_ * (b * b - bb_);
    }

    // The small constant keeps silence at 0 instead of dividing by zero.
    float value() const { return ab_ / std::sqrt(aa_ * bb_ + 1e-12f); }

private:
    float lambda_;
    float ab_ = 0.0f;
    float aa_ = 0.0f;
    float bb_ = 0.0f;
};

}  // namespace anc
