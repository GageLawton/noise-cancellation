#include <algorithm>
#include <cmath>

#include "anc/dc_blocker.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

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

void test_settles_after_sudden_offset() {
    // A step of 0.5 decays as 0.995^n, so 1000 samples (1/8 s at 8 kHz)
    // leaves about 0.003.
    anc::DcBlocker blocker;
    float output = 0.0f;
    for (int sample = 0; sample < 1000; ++sample) {
        output = blocker.process(0.5f);
    }
    TEST_ASSERT_TRUE(std::abs(output) < 0.01f);
}

void test_keeps_60_hz_hum() {
    anc::DcBlocker blocker;
    float peak             = 0.0f;
    constexpr float kTwoPi = 6.28318530718f;
    for (int sample = 0; sample < 8000; ++sample) {
        const float input  = 0.5f * std::sin(kTwoPi * 60.0f * sample / 8000.0f);
        const float output = blocker.process(input);
        if (sample >= 1000) {  // Skip the start-up transient.
            peak = std::max(peak, std::abs(output));
        }
    }
    TEST_ASSERT_TRUE(peak > 0.45f);
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
    RUN_TEST(test_passes_a_tone);
    RUN_TEST(test_settles_after_sudden_offset);
    RUN_TEST(test_keeps_60_hz_hum);
    RUN_TEST(test_reset_clears_state);
    return UNITY_END();
}
