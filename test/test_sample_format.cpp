#include "anc/sample_format.hpp"
#include "unity.h"

#include <cstdint>

void setUp() {}
void tearDown() {}

namespace {

void test_zero_sample_is_zero() {
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, anc::micSampleToFloat(0));
}

void test_minimum_sample_is_minus_one() {
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, -1.0f, anc::micSampleToFloat(INT32_MIN));
}

void test_maximum_sample_is_just_below_one() {
    const float sample = anc::micSampleToFloat(0x7FFFFF00);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.99999988f, sample);
}

void test_unused_low_bits_are_ignored() {
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, anc::micSampleToFloat(0x000000FF));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_zero_sample_is_zero);
    RUN_TEST(test_minimum_sample_is_minus_one);
    RUN_TEST(test_maximum_sample_is_just_below_one);
    RUN_TEST(test_unused_low_bits_are_ignored);
    return UNITY_END();
}
