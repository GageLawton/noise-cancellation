#include <cmath>

#include "anc/probe_generator.hpp"
#include "anc/rolling_correlation.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr double kTwoPi = 6.283185307179586;

float sine100(int n) {
    return static_cast<float>(std::sin(kTwoPi * 100.0 * n / 8000.0));
}

void test_signal_against_itself_is_one() {
    anc::RollingCorrelation corr(0.001f);
    for (int n = 0; n < 8000; ++n) {
        corr.update(sine100(n), sine100(n));
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, corr.value());
}

void test_signal_against_its_negative_is_minus_one() {
    anc::RollingCorrelation corr(0.001f);
    for (int n = 0; n < 8000; ++n) {
        corr.update(sine100(n), -sine100(n));
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, corr.value());
}

void test_unrelated_noise_is_near_zero() {
    anc::RollingCorrelation corr(0.001f);
    anc::ProbeGenerator a(1, 1.0f);
    anc::ProbeGenerator b(2, 1.0f);
    for (int n = 0; n < 8000; ++n) {
        corr.update(a.next(), b.next());
    }
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, corr.value());
}

void test_silence_is_zero() {
    anc::RollingCorrelation corr(0.001f);
    corr.update(0.0f, 0.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, corr.value());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_signal_against_itself_is_one);
    RUN_TEST(test_signal_against_its_negative_is_minus_one);
    RUN_TEST(test_unrelated_noise_is_near_zero);
    RUN_TEST(test_silence_is_zero);
    return UNITY_END();
}
