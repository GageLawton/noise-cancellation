#pragma once

namespace anc {

// Removes constant microphone offset. With r=0.995 at 8 kHz the cutoff
// is about 6 Hz, well below the 60 Hz hum we want to preserve.
// Choose 0 <= r < 1 for a decaying response to a constant input.
class DcBlocker {
public:
    explicit DcBlocker(float r = 0.995f) : r_(r) {}

    float process(float x) {
        const float y = x - x1_ + r_ * y1_;
        x1_           = x;
        y1_           = y;
        return y;
    }

    void reset() {
        x1_ = 0.0f;
        y1_ = 0.0f;
    }

private:
    float r_;
    float x1_ = 0.0f;  // Previous input.
    float y1_ = 0.0f;  // Previous output.
};

}  // namespace anc
