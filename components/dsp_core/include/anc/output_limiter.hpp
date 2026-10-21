#pragma once

#include <cmath>

namespace anc {

// Safety limit on the speaker output (design doc section 8). It works
// independently of the filter math: whatever the canceller asks for, the
// sample that reaches the amp never goes past +/- ceiling.
//
// Clamps x to [-ceiling, ceiling]. Sets clipped to true when it had to, and
// to false when x was already inside the limit. The caller counts the true
// results to fill NodeStatus::clipEvents. NaN fails quiet: a NaN fails both
// clamp comparisons and would pass straight to the speaker, so it is turned
// into silence (0.0) and counted as a clip event.
inline float limitOutput(float x, float ceiling, bool& clipped) {
    if (std::isnan(x)) {
        clipped = true;
        return 0.0f;
    }
    if (x > ceiling) {
        clipped = true;
        return ceiling;
    }
    if (x < -ceiling) {
        clipped = true;
        return -ceiling;
    }
    clipped = false;
    return x;
}

}  // namespace anc
