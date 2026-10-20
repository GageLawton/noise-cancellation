#include <cmath>

#include "anc/peak.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_peak_is_largest_magnitude() {
    const float samples[] = {0.1f, -0.7f, 0.3f};
    TEST_ASSERT_EQUAL_FLOAT(0.7f, anc::peakAbs(samples, 3));
}

void test_square_wave_crest_factor_is_zero_db() {
    float samples[64];
    for (int i = 0; i < 64; ++i)
        samples[i] = (i % 2 == 0) ? 0.5f : -0.5f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, anc::crestFactorDb(samples, 64));
}

void test_sine_crest_factor_is_three_db() {
    // 800 samples of 60 Hz at 8000 Hz is six whole cycles.
    float samples[800];
    constexpr double kTwoPi = 6.283185307179586;
    for (int i = 0; i < 800; ++i)
        samples[i] = static_cast<float>(std::sin(kTwoPi * 60.0 * i / 8000.0));
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 3.01f, anc::crestFactorDb(samples, 800));
}

void test_silence_is_zero() {
    const float samples[] = {0.0f, 0.0f, 0.0f, 0.0f};
    TEST_ASSERT_EQUAL_FLOAT(0.0f, anc::peakAbs(samples, 4));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, anc::crestFactorDb(samples, 4));
}

void test_empty_block_is_zero() {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, anc::peakAbs(nullptr, 0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, anc::crestFactorDb(nullptr, 0));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_peak_is_largest_magnitude);
    RUN_TEST(test_square_wave_crest_factor_is_zero_db);
    RUN_TEST(test_sine_crest_factor_is_three_db);
    RUN_TEST(test_silence_is_zero);
    RUN_TEST(test_empty_block_is_zero);
    return UNITY_END();
}
