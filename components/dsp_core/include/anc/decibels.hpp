#pragma once

#include <cmath>

namespace anc {

// 20*log10(a). Values at or below 1e-6 return -120 dB instead of -infinity.
inline float amplitudeToDb(float a) {
    return a <= 1e-6f ? -120.0f : 20.0f * std::log10(a);
}

// Converts decibels to amplitude: 10^(db/20).
// The round trip matches amplitudeToDb above its -120 dB reporting floor.
inline float dbToAmplitude(float db) {
    return std::pow(10.0f, db / 20.0f);
}

// Converts a power (a mean square, such as RunningPower's value) to
// decibels: 10*log10(p), with the same -120 dB floor as amplitudeToDb.
inline float powerToDb(float p) {
    return p <= 1e-12f ? -120.0f : 10.0f * std::log10(p);
}

}  // namespace anc
