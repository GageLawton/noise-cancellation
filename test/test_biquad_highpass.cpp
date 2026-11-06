#include <algorithm>
#include <cmath>

#include "anc/biquad.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr float kSampleRateHz = 8000.0f;
constexpr float kCutoffHz     = 30.0f;
constexpr float kQ            = 0.707f;

anc::Biquad makeFilter() {
    return anc::Biquad::highpass(kCutoffHz, kQ, kSampleRateHz);
}

// Runs 2 s of a full-scale sine and returns the peak output over the
// second second, once the filter has settled.
float peakOut(anc::Biquad& filter, float toneHz) {
    constexpr double kTwoPi = 6.283185307179586;
    const int n             = static_cast<int>(kSampleRateHz);
    float peak              = 0.0f;
    for (int sample = 0; sample < 2 * n; ++sample) {
        const float x =
            static_cast<float>(std::sin(kTwoPi * toneHz * sample / kSampleRateHz));
        const float y = filter.process(x);
        if (sample >= n) {
            peak = std::max(peak, std::abs(y));
        }
    }
    return peak;
}

void test_constant_input_settles_to_zero() {
    anc::Biquad filter = makeFilter();
    float y            = 1.0f;
    for (int i = 0; i < 2000; ++i) {
        y = filter.process(1.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, y);
}

void test_keeps_tones_well_above_cutoff() {
    anc::Biquad filter = makeFilter();
    TEST_ASSERT_TRUE(peakOut(filter, 200.0f) >= 0.95f);
}

void test_cuts_rumble_below_cutoff() {
    anc::Biquad filter = makeFilter();
    TEST_ASSERT_TRUE(peakOut(filter, 10.0f) < 0.2f);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_constant_input_settles_to_zero);
    RUN_TEST(test_keeps_tones_well_above_cutoff);
    RUN_TEST(test_cuts_rumble_below_cutoff);
    return UNITY_END();
}
