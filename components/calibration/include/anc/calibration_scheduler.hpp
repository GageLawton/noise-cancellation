#pragma once

#include <cstdint>

namespace anc {

// Decides when to recalibrate (design doc 6.1; config keys
// backstop_interval_ms and cooldown_ms): when drift asks for it, or when
// the backstop interval has passed, but never sooner than the cooldown
// after the last calibration. Works from timestamps alone, so it is easy
// to test.
class CalibrationScheduler {
public:
    CalibrationScheduler(uint32_t backstopMs, uint32_t cooldownMs)
        : backstopMs_(backstopMs), cooldownMs_(cooldownMs) {}

    void markCalibrated(uint32_t nowMs) {
        lastMs_     = nowMs;
        calibrated_ = true;
    }

    // Never calibrated yet means calibrate now. Unsigned subtraction keeps
    // the elapsed time right when the 32-bit millisecond counter wraps
    // (about every 49.7 days).
    bool shouldRecheck(uint32_t nowMs, bool driftRequested) const {
        if (!calibrated_)
            return true;
        const uint32_t elapsed = nowMs - lastMs_;
        if (elapsed < cooldownMs_)
            return false;
        return driftRequested || elapsed >= backstopMs_;
    }

private:
    uint32_t backstopMs_;
    uint32_t cooldownMs_;
    uint32_t lastMs_ = 0;
    bool calibrated_ = false;
};

}  // namespace anc
