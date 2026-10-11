#pragma once

namespace anc {

// Clamps x to [-ceiling, ceiling]. Sets clipped to true when it had to.
// Precondition: x is finite and ceiling is finite and nonnegative.
inline float limitOutput(float x, float ceiling, bool& clipped) {
    clipped = false;

    if (x > ceiling) {
        clipped = true;
        return ceiling;
    }

    if (x < -ceiling) {
        clipped = true;
        return -ceiling;
    }

    return x;
}

}  // namespace anc
