#include <cmath>

#include "anc/rms.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_constant_half_amplitude() {
    const float samples[] = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
    TEST_ASSERT_EQUAL_FLOAT(0.5f, anc::rms(samples, 8));
}

void test_alternating_signs_have_unit_rms() {
    const float samples[] = {1.0f, -1.0f, 1.0f, -1.0f};
    TEST_ASSERT_EQUAL_FLOAT(1.0f, anc::rms(samples, 4));
}

void test_sine_rms_is_inverse_square_root_of_two() {
    float samples[8000];
    constexpr double kTwoPi = 6.283185307179586;
    for (int i = 0; i < 8000; ++i) {
        samples[i] = static_cast<float>(std::sin(kTwoPi * 100.0 * i / 8000.0));
    }
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.7071f, anc::rms(samples, 8000));
}

void test_empty_block_is_zero() {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, anc::rms(nullptr, 0));
}

void test_squares_are_computed_in_double() {
    // These finite floats would overflow if squared before conversion to double.
    const float samples[] = {1e20f, -1e20f};
    const float result    = anc::rms(samples, 2);
    TEST_ASSERT_TRUE(std::isfinite(result));
    TEST_ASSERT_FLOAT_WITHIN(1e14f, 1e20f, result);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_constant_half_amplitude);
    RUN_TEST(test_alternating_signs_have_unit_rms);
    RUN_TEST(test_sine_rms_is_inverse_square_root_of_two);
    RUN_TEST(test_empty_block_is_zero);
    RUN_TEST(test_squares_are_computed_in_double);
    return UNITY_END();
}
