#include <algorithm>
#include <cmath>

#include "anc/biquad.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr float kSampleRateHz = 8000.0f;
constexpr float kQ            = 5.0f;

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

void test_bandpass_keeps_the_hum() {
    anc::Biquad filter = anc::Biquad::bandpass(60.0f, kQ, kSampleRateHz);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, peakOut(filter, 60.0f));
}

void test_bandpass_rejects_far_tones() {
    anc::Biquad filter = anc::Biquad::bandpass(60.0f, kQ, kSampleRateHz);
    TEST_ASSERT_TRUE(peakOut(filter, 600.0f) < 0.05f);
}

void test_notch_removes_the_hum() {
    anc::Biquad filter = anc::Biquad::notch(60.0f, kQ, kSampleRateHz);
    TEST_ASSERT_TRUE(peakOut(filter, 60.0f) < 0.01f);
}

void test_notch_passes_far_tones() {
    anc::Biquad filter = anc::Biquad::notch(60.0f, kQ, kSampleRateHz);
    TEST_ASSERT_TRUE(peakOut(filter, 600.0f) > 0.95f);
}

void test_reset_clears_state() {
    anc::Biquad filter = anc::Biquad::bandpass(60.0f, kQ, kSampleRateHz);
    filter.process(1.0f);
    filter.process(-0.5f);
    filter.reset();
    for (int sample = 0; sample < 10; ++sample) {
        TEST_ASSERT_EQUAL_FLOAT(0.0f, filter.process(0.0f));
    }
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_bandpass_keeps_the_hum);
    RUN_TEST(test_bandpass_rejects_far_tones);
    RUN_TEST(test_notch_removes_the_hum);
    RUN_TEST(test_notch_passes_far_tones);
    RUN_TEST(test_reset_clears_state);
    return UNITY_END();
}
