#pragma once

#include <cmath>

namespace anc {

// Phase-locked loop that follows a tone's frequency as it drifts (mains
// wander, fans changing speed), so the canceller stays on target.
//
// Per sample it mixes the input with its own oscillator, low-passes the
// products into i and q, and reads the phase error as atan2(i, q). The
// error nudges the frequency (integral term) and the phase (proportional
// term). For an input A*sin(phi), i ~ A/2 * sin(phi - theta) and
// q ~ A/2 * cos(phi - theta), so the error is phi - theta.
class Pll {
public:
    Pll(float initialFreqHz, float sampleRateHz)
        : freqHz_(initialFreqHz), sampleRateHz_(sampleRateHz) {}

    // Self-lock guard (design doc 6.1): real hums drift slowly, so the
    // frequency may change by at most hzPerSec. A sudden jump is more
    // likely the loop grabbing our own speaker than a real change.
    // No limit until this is called.
    void setMaxDriftRate(float hzPerSec) {
        maxStepHz_ = hzPerSec / sampleRateHz_;
        limited_   = true;
    }

    void update(float x) {
        i_ += kAlpha * (x * std::cos(theta_) - i_);
        q_ += kAlpha * (x * std::sin(theta_) - q_);
        err_         = std::atan2(i_, q_);
        float stepHz = kKi * err_;
        if (limited_) {
            stepHz = std::fmin(std::fmax(stepHz, -maxStepHz_), maxStepHz_);
        }
        freqHz_ += stepHz;
        theta_ += kTwoPi * (freqHz_ + kKp * err_) / sampleRateHz_;
        theta_ = std::fmod(theta_, kTwoPi);
        if (theta_ < 0.0f) {
            theta_ += kTwoPi;
        }
    }

    float frequencyHz() const { return freqHz_; }
    float phaseError() const { return err_; }

private:
    static constexpr float kTwoPi = 6.28318530718f;
    static constexpr float kAlpha = 0.005f;  // i/q low-pass
    static constexpr float kKi    = 0.002f;  // frequency gain, Hz per radian
    static constexpr float kKp    = 2.0f;    // phase gain, Hz per radian

    float freqHz_;
    float sampleRateHz_;
    float theta_     = 0.0f;  // oscillator phase, kept in [0, 2*pi)
    float i_         = 0.0f;
    float q_         = 0.0f;
    float err_       = 0.0f;
    bool limited_    = false;
    float maxStepHz_ = 0.0f;  // per-sample frequency limit, when limited_
};

}  // namespace anc
