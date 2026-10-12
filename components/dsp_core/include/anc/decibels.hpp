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

}  // namespace anc
