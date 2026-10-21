#pragma once

#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "anc/node_config.hpp"

namespace anc {

// Maps "section.key" names (the same keys as node.json) to NodeConfig
// fields, for the serial `set` and `get` commands.
namespace config_keys {

template <typename T>
struct Field {
    const char* name;
    T NodeConfig::*member;
};

inline constexpr Field<float> kFloatFields[] = {
    {"dsp.normalized_step_size", &NodeConfig::normalizedStepSize},
    {"calibration.correlation_threshold", &NodeConfig::correlationThreshold},
    {"tracking.max_drift_rate_hz_per_sec", &NodeConfig::maxDriftRateHzPerSec},
    {"tracking.watch_correlation_threshold", &NodeConfig::watchCorrelationThreshold},
    {"tracking.min_lock_amplitude", &NodeConfig::minLockAmplitude},
    {"output.ceiling", &NodeConfig::outputCeiling},
    {"output.test_tone_hz", &NodeConfig::testToneHz},
    {"output.test_tone_amplitude", &NodeConfig::testToneAmplitude},
};

inline constexpr Field<uint16_t> kU16Fields[] = {
    {"dsp.secondary_path_taps", &NodeConfig::secondaryPathTaps},
    {"dsp.dma_block_samples", &NodeConfig::dmaBlockSamples},
};

inline constexpr Field<uint32_t> kU32Fields[] = {
    {"calibration.probe_duration_ms", &NodeConfig::probeDurationMs},
    {"calibration.correlation_window_ms", &NodeConfig::correlationWindowMs},
    {"calibration.backstop_interval_ms", &NodeConfig::backstopIntervalMs},
    {"calibration.cooldown_ms", &NodeConfig::cooldownMs},
};

// Returns the matching field, or nullptr.
template <typename T, std::size_t N>
const Field<T>* find(const Field<T> (&fields)[N], const char* key) {
    for (const Field<T>& f : fields) {
        if (std::strcmp(f.name, key) == 0) {
            return &f;
        }
    }
    return nullptr;
}

// The whole string must be a finite number.
inline bool parseFloat(const char* text, float& out) {
    char* end    = nullptr;
    errno        = 0;
    const auto v = std::strtof(text, &end);
    if (end == text || *end != '\0' || errno == ERANGE || !std::isfinite(v))
        return false;
    out = v;
    return true;
}

// The whole string must be a non-negative integer no bigger than max.
// strtoul would quietly turn "-1" into a huge number, so a sign is refused.
inline bool parseUnsigned(const char* text, unsigned long max, unsigned long& out) {
    if (text[0] == '-' || text[0] == '+')
        return false;
    char* end    = nullptr;
    errno        = 0;
    const auto v = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || errno == ERANGE || v > max)
        return false;
    out = v;
    return true;
}

}  // namespace config_keys

// Parses value, writes it to a copy of cfg, and keeps it only if the copy
// passes validate(). Returns false (cfg unchanged) for an unknown key, an
// unparsable value, or a value that fails validation.
//
// identity.node_name and tracking.initial_freq_hz are not covered yet.
inline bool applySetting(NodeConfig& cfg, const char* key, const char* value) {
    using namespace config_keys;
    if (key == nullptr || value == nullptr)
        return false;

    NodeConfig candidate = cfg;
    if (const auto* f = find(kFloatFields, key)) {
        float v = 0.0f;
        if (!parseFloat(value, v))
            return false;
        candidate.*(f->member) = v;
    } else if (const auto* f16 = find(kU16Fields, key)) {
        unsigned long v = 0;
        if (!parseUnsigned(value, UINT16_MAX, v))
            return false;
        candidate.*(f16->member) = static_cast<uint16_t>(v);
    } else if (const auto* f32 = find(kU32Fields, key)) {
        unsigned long v = 0;
        if (!parseUnsigned(value, UINT32_MAX, v))
            return false;
        candidate.*(f32->member) = static_cast<uint32_t>(v);
    } else {
        return false;
    }

    if (!candidate.validate())
        return false;
    cfg = candidate;
    return true;
}

// Writes the current value of key into out: "%g" for floats (so a small
// step size like 0.0025 isn't rounded to 0.003), "%u" for integers. Returns false for
// an unknown key, or if out is too small (it then holds as much as fits).
inline bool formatSetting(const NodeConfig& cfg, const char* key, char* out,
                          std::size_t outLen) {
    using namespace config_keys;
    if (key == nullptr || out == nullptr || outLen == 0)
        return false;

    int written = -1;
    if (const auto* f = find(kFloatFields, key)) {
        written =
            std::snprintf(out, outLen, "%g", static_cast<double>(cfg.*(f->member)));
    } else if (const auto* f16 = find(kU16Fields, key)) {
        written =
            std::snprintf(out, outLen, "%u", static_cast<unsigned>(cfg.*(f16->member)));
    } else if (const auto* f32 = find(kU32Fields, key)) {
        written = std::snprintf(out, outLen, "%lu",
                                static_cast<unsigned long>(cfg.*(f32->member)));
    } else {
        out[0] = '\0';
        return false;
    }
    return written >= 0 && static_cast<std::size_t>(written) < outLen;
}

}  // namespace anc
