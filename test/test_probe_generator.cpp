#include <cmath>

#include "anc/probe_generator.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_same_seed_gives_same_sequence() {
    anc::ProbeGenerator a(1234, 0.5f);
    anc::ProbeGenerator b(1234, 0.5f);
    for (int i = 0; i < 1000; ++i) {
        // Unity macros may evaluate arguments more than once, so take the
        // samples first.
        const float fromA = a.next();
        const float fromB = b.next();
        TEST_ASSERT_EQUAL_FLOAT(fromA, fromB);
    }
}

void test_zero_seed_still_produces_noise() {
    anc::ProbeGenerator gen(0, 1.0f);
    bool sawNonZero = false;
    for (int i = 0; i < 10; ++i) {
        sawNonZero = sawNonZero || gen.next() != 0.0f;
    }
    TEST_ASSERT_TRUE(sawNonZero);
}

void test_values_stay_within_amplitude() {
    anc::ProbeGenerator gen(7, 0.3f);
    for (int i = 0; i < 10000; ++i) {
        const float x = gen.next();
        TEST_ASSERT_TRUE(x >= -0.3f && x <= 0.3f);
    }
}

void test_mean_is_near_zero() {
    anc::ProbeGenerator gen(42, 1.0f);
    double sum = 0.0;
    for (int i = 0; i < 10000; ++i) {
        sum += gen.next();
    }
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.0f, static_cast<float>(sum / 10000.0));
}

void test_rms_matches_uniform_noise() {
    anc::ProbeGenerator gen(42, 1.0f);
    double sumSquares = 0.0;
    for (int i = 0; i < 10000; ++i) {
        const double x = gen.next();
        sumSquares += x * x;
    }
    // Uniform noise on [-1, 1) has an RMS of 1 / sqrt(3).
    const float rms = static_cast<float>(std::sqrt(sumSquares / 10000.0));
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.577f, rms);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_same_seed_gives_same_sequence);
    RUN_TEST(test_zero_seed_still_produces_noise);
    RUN_TEST(test_values_stay_within_amplitude);
    RUN_TEST(test_mean_is_near_zero);
    RUN_TEST(test_rms_matches_uniform_noise);
    return UNITY_END();
}
