#include <algorithm>
#include <cmath>

#include "anc/dc_blocker.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_step_offset_decays_below_threshold_in_1000_samples() {
    anc::DcBlocker blocker;
    float output = 0.0f;
    for (int sample = 0; sample < 8000; ++sample) {
        output = blocker.process(0.5f);
        if (sample == 999) {
            // r = 0.995: a step decays as 0.995^n, so 1000 samples leave ~0.5 * 0.0067.
            TEST_ASSERT_TRUE(std::abs(output) < 0.01f);
        }
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

void test_60hz_hum_amplitude_is_preserved_after_settling() {
    anc::DcBlocker blocker;
    constexpr float kTwoPi      = 6.28318530718f;
    constexpr float kSampleRate = 8000.0f;
    constexpr float kAmplitude  = 0.5f;
    float peak                  = 0.0f;
    for (int sample = 0; sample < 8000; ++sample) {
        const float input =
            kAmplitude * std::sin(kTwoPi * 60.0f * sample / kSampleRate);
        const float output = blocker.process(input);
        if (sample >= 1000) {  // skip the settling window
            peak = std::max(peak, std::abs(output));
        }
    }
    TEST_ASSERT_TRUE(peak > 0.45f);
}

void test_removes_constant_offset() {
    anc::DcBlocker blocker;
    float output = 0.0f;
    for (int sample = 0; sample < 8000; ++sample) {
        output = blocker.process(0.3f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

void test_passes_a_tone() {
    anc::DcBlocker blocker;
    float peak             = 0.0f;
    constexpr float kTwoPi = 6.28318530718f;
    for (int sample = 0; sample < 8000; ++sample) {
        const float input  = std::sin(kTwoPi * 1000.0f * sample / 8000.0f);
        const float output = blocker.process(input);
        if (sample >= 7900) {
            peak = std::max(peak, std::abs(output));
        }
    }
    TEST_ASSERT_TRUE(peak > 0.95f);
}

void test_reset_clears_state() {
    anc::DcBlocker blocker;
    blocker.process(0.3f);
    blocker.process(-0.2f);
    blocker.reset();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, blocker.process(0.0f));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_removes_constant_offset);
    RUN_TEST(test_step_offset_decays_below_threshold_in_1000_samples);
    RUN_TEST(test_60hz_hum_amplitude_is_preserved_after_settling);
    RUN_TEST(test_passes_a_tone);
    RUN_TEST(test_reset_clears_state);
    return UNITY_END();
}
