#pragma once

namespace anc {

// Exponential running average of signal power (design doc 6.1):
//   P(n) = (1 - lambda) * P(n-1) + lambda * x(n)^2
// A small lambda averages over roughly 1 / lambda samples.
class RunningPower {
public:
    explicit RunningPower(float lambda) : lambda_(lambda) {}

    // Adds x^2 to the average and returns the new value.
    float update(float x) {
        power_ += lambda_ * (x * x - power_);
        return power_;
    }

    float value() const { return power_; }
    void reset() { power_ = 0.0f; }

private:
    float lambda_;
    float power_ = 0.0f;
};

}  // namespace anc
