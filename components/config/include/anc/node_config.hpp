#pragma once

#include <cstdint>

namespace anc {

// Maximum simultaneously tracked tones (fundamental + harmonics).
// Compile-time because it sizes fixed arrays in the real-time path.
inline constexpr int kMaxTones = 4;

inline constexpr int kNodeNameMaxLen = 32;

// Sample rate is compile-time: changing it invalidates every tuned
// threshold in the config, so it is deliberately not runtime-adjustable.
inline constexpr float kSampleRateHz = 8000.0f;

// NLMS regularization floor. A numerical-safety constant, not a
// bench-tuning parameter -- see design doc section 5.7.
inline constexpr float kNlmsEpsilon = 1e-6f;

// Runtime-adjustable configuration, persisted as JSON on flash.
// Every field here can be changed without recompiling.
struct NodeConfig {
    // --- identity ---
    // Human-readable label for logs only. Never used in protocol logic;
    // machine identity is the MAC-derived node_id (see NodeRuntime).
    char nodeName[kNodeNameMaxLen];

    // --- dsp ---
    uint16_t secondaryPathTaps;    // SecondaryPath model length
    float    normalizedStepSize;   // NLMS mu, dimensionless, (0, 2)
    uint16_t dmaBlockSamples;      // I2S DMA block size

    // --- calibration ---
    uint32_t probeDurationMs;
    float    correlationThreshold;     // fires a calibration event
    uint32_t correlationWindowMs;
    uint32_t backstopIntervalMs;       // max time between calibrations
    uint32_t cooldownMs;               // min time between calibrations

    // --- tracking ---
    float    initialFreqHz[kMaxTones];  // PLL starting hints
    uint8_t  numTones;
    float    maxDriftRateHzPerSec;      // self-lock rate limit
    float    watchCorrelationThreshold; // low-confidence gate

    // Compiled-in fallback, used on first flash or if the JSON file
    // is missing/corrupt. Always writable back out as a fresh file.
    static NodeConfig defaults();

    // Returns false (and leaves the config untouched) if any field is
    // out of a physically sensible range.
    bool validate() const;
};

}  // namespace anc
