#pragma once

#include <cstddef>
#include <cstdint>

#include "anc/drift_detector.hpp"
#include "anc/node_config.hpp"
#include "anc/pll.hpp"
#include "anc/reconstructor.hpp"

namespace anc {

// What AlignmentTask drives (design doc 10.3). Each {mic, synth} pair goes
// through the Reconstructor to remove our own speaker; the estimated noise
// then feeds the DriftDetector (is the path model stale?) and one Pll per
// tone (where has the hum drifted to?).
class AlignmentEngine {
public:
    static constexpr std::size_t kTones = kMaxTones;

    // Call before update(). Sets the path, the drift thresholds and window
    // from the config, and starts cfg.numTones Plls at cfg.initialFreqHz.
    void configure(const NodeConfig& cfg, const float* path, std::size_t taps) {
        reconstructor_.setSecondaryPath(path, taps);

        // The config has one window; it serves both as the averaging
        // window and as how long the trigger must hold.
        const std::size_t window = msToSamples(cfg.correlationWindowMs);
        drift_ = DriftDetector(cfg.correlationThreshold, cfg.watchCorrelationThreshold,
                               window, window);

        numTones_ = cfg.numTones < kTones ? cfg.numTones : kTones;
        for (std::size_t t = 0; t < numTones_; ++t) {
            plls_[t] = Pll(cfg.initialFreqHz[t], kSampleRateHz);
            plls_[t].setMaxDriftRate(cfg.maxDriftRateHzPerSec);
            // Halved because the PLL's i/q amplitude is about half the
            // hum's amplitude (see setMinAmplitude in pll.hpp).
            plls_[t].setMinAmplitude(cfg.minLockAmplitude / 2.0f);
        }
    }

    // One mic sample and the speaker sample played at the same time.
    void update(float mic, float synth) {
        const float noise = reconstructor_.process(mic, synth);
        drift_.update(noise, synth);
        for (std::size_t t = 0; t < numTones_; ++t) {
            plls_[t].update(noise);
        }
    }

    std::size_t numTones() const { return numTones_; }
    float frequencyHz(std::size_t tone) const { return plls_[tone].frequencyHz(); }
    bool locked(std::size_t tone) const { return plls_[tone].locked(); }
    bool needsCalibration() const { return drift_.needsCalibration(); }
    bool lowConfidence() const { return drift_.lowConfidence(); }
    float correlation() const { return drift_.correlation(); }

private:
    static std::size_t msToSamples(uint32_t ms) {
        return static_cast<std::size_t>(static_cast<float>(ms) * kSampleRateHz /
                                        1000.0f);
    }

    Reconstructor reconstructor_;
    // Placeholders until configure() replaces them.
    DriftDetector drift_{0.3f, 0.15f, 8000, 8000};
    Pll plls_[kTones]     = {Pll(0.0f, kSampleRateHz), Pll(0.0f, kSampleRateHz),
                             Pll(0.0f, kSampleRateHz), Pll(0.0f, kSampleRateHz)};
    std::size_t numTones_ = 0;

    static_assert(kTones == 4, "update the plls_ initializer to match kMaxTones");
};

}  // namespace anc
