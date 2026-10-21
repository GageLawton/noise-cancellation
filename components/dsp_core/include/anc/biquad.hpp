#pragma once

#include <cmath>

namespace anc {

// Two-pole, two-zero filter (Direct Form I). The factories use the Audio
// EQ Cookbook formulas (https://www.w3.org/TR/audio-eq-cookbook/): a
// bandpass at the hum keeps just the hum, a notch removes it.
class Biquad {
public:
    // "BPF, constant 0 dB peak gain": unity gain at centerHz.
    static Biquad bandpass(float centerHz, float q, float sampleRateHz) {
        const Shape s = shape(centerHz, q, sampleRateHz);
        return Biquad(s.alpha, 0.0, -s.alpha, s);
    }

    // Zero gain at centerHz, unity gain far from it.
    static Biquad notch(float centerHz, float q, float sampleRateHz) {
        const Shape s = shape(centerHz, q, sampleRateHz);
        return Biquad(1.0, -2.0 * s.cosW0, 1.0, s);
    }

    float process(float x) {
        const float y = b0_ * x + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
        x2_           = x1_;
        x1_           = x;
        y2_           = y1_;
        y1_           = y;
        return y;
    }

    void reset() {
        x1_ = 0.0f;
        x2_ = 0.0f;
        y1_ = 0.0f;
        y2_ = 0.0f;
    }

private:
    // The cookbook's intermediate values, shared by both shapes.
    struct Shape {
        double cosW0;
        double alpha;
    };

    static Shape shape(float centerHz, float q, float sampleRateHz) {
        constexpr double kTwoPi = 6.283185307179586;
        const double w0         = kTwoPi * centerHz / sampleRateHz;
        return {std::cos(w0), std::sin(w0) / (2.0 * q)};
    }

    // Both shapes share the same poles; every coefficient is divided by a0.
    Biquad(double b0, double b1, double b2, const Shape& s) {
        const double a0 = 1.0 + s.alpha;
        b0_             = static_cast<float>(b0 / a0);
        b1_             = static_cast<float>(b1 / a0);
        b2_             = static_cast<float>(b2 / a0);
        a1_             = static_cast<float>(-2.0 * s.cosW0 / a0);
        a2_             = static_cast<float>((1.0 - s.alpha) / a0);
    }

    float b0_ = 0.0f, b1_ = 0.0f, b2_ = 0.0f;
    float a1_ = 0.0f, a2_ = 0.0f;
    float x1_ = 0.0f, x2_ = 0.0f;  // Previous two inputs.
    float y1_ = 0.0f, y2_ = 0.0f;  // Previous two outputs.
};

}  // namespace anc
