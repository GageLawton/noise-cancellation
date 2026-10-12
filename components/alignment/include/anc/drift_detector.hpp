#pragma once

#include <cmath>
#include <cstddef>

#include "anc/rolling_correlation.hpp"

namespace anc {

// Watches for a stale speaker-to-mic model (design doc 6.1, "calibration
// trigger" and "self-lock safeguard"). If the estimated noise still holds
// a copy of our own output, the model is off. Two thresholds:
//   - watch: above it, tracking is low-confidence;
//   - trigger: above it for sustainSamples in a row, recalibrate.
class DriftDetector {
public:
    DriftDetector(float triggerThreshold, float watchThreshold,
                  std::size_t windowSamples, std::size_t sustainSamples)
        : trigger_(triggerThreshold),
          watch_(watchThreshold),
          sustain_(sustainSamples),
          corr_(1.0f / static_cast<float>(windowSamples)) {}

    void update(float estimatedNoise, float synthOutput) {
        corr_.update(estimatedNoise, synthOutput);
        if (std::fabs(corr_.value()) > trigger_) {
            if (aboveCount_ < sustain_) {
                ++aboveCount_;
            }
        } else {
            aboveCount_ = 0;
        }
    }

    float correlation() const { return corr_.value(); }
    bool lowConfidence() const { return std::fabs(corr_.value()) > watch_; }
    bool needsCalibration() const { return aboveCount_ >= sustain_; }

private:
    float trigger_;
    float watch_;
    std::size_t sustain_;
    RollingCorrelation corr_;
    std::size_t aboveCount_ = 0;
};

}  // namespace anc
