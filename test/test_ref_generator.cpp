#include "anc/ref_generator.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_first_sample_is_zero_sine_and_one_cosine() {
    anc::RefGenerator generator(8000.0f);
    generator.setFrequency(60.0f);

    float sinOut = -99.0f;
    float cosOut = -99.0f;
    generator.next(sinOut, cosOut);

    TEST_ASSERT_EQUAL_FLOAT(0.0f, sinOut);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, cosOut);
}

void test_1khz_repeats_every_8_samples() {
    anc::RefGenerator generator(8000.0f);
    generator.setFrequency(1000.0f);

    float sinOut = 0.0f;
    float cosOut = 0.0f;
    generator.next(sinOut, cosOut);  // Sample 0.
    const float firstSin = sinOut;
    const float firstCos = cosOut;

    generator.next(sinOut, cosOut);  // Sample 1.
    generator.next(sinOut, cosOut);  // Sample 2.
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, sinOut);

    for (int sample = 3; sample <= 8; ++sample) {
        generator.next(sinOut, cosOut);
    }

    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, sinOut);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, cosOut);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, firstSin, sinOut);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, firstCos, cosOut);
}

void test_phase_stays_accurate_over_time() {
    anc::RefGenerator generator(8000.0f);
    generator.setFrequency(1000.0f);

    float sinOut = 0.0f;
    float cosOut = 0.0f;
    for (int sample = 0; sample < 80000; ++sample) {
        generator.next(sinOut, cosOut);
    }
    generator.next(sinOut, cosOut);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, sinOut);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, cosOut);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_first_sample_is_zero_sine_and_one_cosine);
    RUN_TEST(test_1khz_repeats_every_8_samples);
    RUN_TEST(test_phase_stays_accurate_over_time);
    return UNITY_END();
}
