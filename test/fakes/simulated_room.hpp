#pragma once

#include <cmath>
#include <cstddef>

#include "anc/fir_filter.hpp"

// Host-only pretend room for testing the canceller on a laptop: up to
// four steady hums plus a fixed speaker-to-mic path (design doc 9.1,
// "simulated SecondaryPath as a fixed FIR").
class SimulatedRoom {
public:
    static constexpr std::size_t kMaxHums = 4;

    SimulatedRoom(float sampleRateHz, const float* pathCoeffs, std::size_t pathTaps)
        : sampleRateHz_(sampleRateHz) {
        path_.setCoefficients(pathCoeffs, pathTaps);
    }

    // Adds a hum of amplitude * sin(2*pi*freq*t + phase). Extra hums past
    // kMaxHums are ignored.
    void addHum(float freqHz, float amplitude, float phaseRad) {
        if (numHums_ < kMaxHums) {
            hums_[numHums_++] = {freqHz, amplitude, phaseRad};
        }
    }

    // Plays one speaker sample and returns what the mic hears:
    // the hums plus the speaker through the path.
    float step(float speakerSample) {
        constexpr double kTwoPi = 6.283185307179586;
        // Time in double, so the hum phase stays accurate over long runs.
        const double t = static_cast<double>(n_++) / sampleRateHz_;
        double mic     = 0.0;
        for (std::size_t h = 0; h < numHums_; ++h) {
            mic += hums_[h].amplitude *
                   std::sin(kTwoPi * hums_[h].freqHz * t + hums_[h].phaseRad);
        }
        return static_cast<float>(mic) + path_.process(speakerSample);
    }

private:
    struct Hum {
        float freqHz;
        float amplitude;
        float phaseRad;
    };

    float sampleRateHz_;
    anc::FirFilter<128> path_;
    Hum hums_[kMaxHums]{};
    std::size_t numHums_ = 0;
    unsigned long n_     = 0;  // sample counter for the hums
};
