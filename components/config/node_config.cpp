#include "anc/node_config.hpp"

#include <cstring>

namespace anc {

NodeConfig NodeConfig::defaults() {
    NodeConfig c{};

    std::strncpy(c.nodeName, "anc-node", kNodeNameMaxLen - 1);

    c.secondaryPathTaps = 256;  // 32ms at 8kHz, covers the loop delay (design doc 6.2)
    // Small on purpose: with ~20 ms of loop delay, FxLMS goes unstable a
    // little above 0.015 (test_fxlms_canceller shows 0.1 diverging).
    c.normalizedStepSize = 0.005f;
    c.dmaBlockSamples    = 32;  // 4ms at 8kHz

    c.probeDurationMs      = 300;
    c.correlationThreshold = 0.3f;
    c.correlationWindowMs  = 1000;
    c.backstopIntervalMs   = 600000;  // 10 min
    c.cooldownMs           = 30000;   // 30 s

    c.numTones         = 1;
    c.initialFreqHz[0] = 60.0f;            // US mains hum -- a starting hint only,
    for (int i = 1; i < kMaxTones; ++i) {  // the PLL walks to the real tone
        c.initialFreqHz[i] = 0.0f;
    }
    c.maxDriftRateHzPerSec      = 2.0f;
    c.watchCorrelationThreshold = 0.15f;
    c.minLockAmplitude          = 0.1f;  // about -20 dBFS, today's PLL lock level

    c.outputCeiling     = 0.8f;
    c.testToneHz        = 440.0f;
    c.testToneAmplitude = 0.1f;

    return c;
}

bool NodeConfig::validate() const {
    if (nodeName[0] == '\0')
        return false;

    if (secondaryPathTaps < 8 || secondaryPathTaps > 1024)
        return false;
    // NLMS stability bound is (0, 2); stay strictly inside it.
    if (normalizedStepSize <= 0.0f || normalizedStepSize >= 2.0f)
        return false;
    if (dmaBlockSamples < 8 || dmaBlockSamples > 256)
        return false;

    if (probeDurationMs < 50 || probeDurationMs > 5000)
        return false;
    if (correlationThreshold <= 0.0f || correlationThreshold >= 1.0f)
        return false;
    if (correlationWindowMs < 100 || correlationWindowMs > 10000)
        return false;
    if (backstopIntervalMs < cooldownMs)
        return false;

    if (numTones < 1 || numTones > kMaxTones)
        return false;
    for (int i = 0; i < numTones; ++i) {
        // Must be a real tone below Nyquist.
        if (initialFreqHz[i] <= 0.0f || initialFreqHz[i] >= kSampleRateHz / 2.0f) {
            return false;
        }
    }
    if (maxDriftRateHzPerSec <= 0.0f)
        return false;
    // The watch gate must trip before the calibration trigger, or it
    // would never mark anything low-confidence.
    if (watchCorrelationThreshold <= 0.0f ||
        watchCorrelationThreshold >= correlationThreshold) {
        return false;
    }
    if (minLockAmplitude <= 0.0f || minLockAmplitude > 1.0f)
        return false;

    if (outputCeiling <= 0.0f || outputCeiling > 1.0f)
        return false;
    if (testToneHz <= 0.0f || testToneHz >= kSampleRateHz / 2.0f)
        return false;
    // The tone must fit under the limiter, or the limiter would clip it.
    if (testToneAmplitude <= 0.0f || testToneAmplitude > outputCeiling)
        return false;

    return true;
}

}  // namespace anc
