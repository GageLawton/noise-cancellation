#include <cmath>

#include "anc/running_power.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_starts_at_zero() {
    anc::RunningPower power(0.01f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, power.value());
}

void test_constant_input_settles_to_its_square() {
    anc::RunningPower power(0.01f);
    float last = 0.0f;
    for (int i = 0; i < 1000; ++i) {
        last = power.update(1.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, power.value());
    TEST_ASSERT_EQUAL_FLOAT(last, power.value());
}

void test_unit_sine_settles_to_one_half() {
    anc::RunningPower power(0.01f);
    constexpr double kTwoPi = 6.283185307179586;
    for (int i = 0; i < 8000; ++i) {
        power.update(static_cast<float>(std::sin(kTwoPi * 60.0 * i / 8000.0)));
    }
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.5f, power.value());
}

void test_reset_returns_to_zero() {
    anc::RunningPower power(0.01f);
    power.update(3.0f);
    power.reset();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, power.value());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_starts_at_zero);
    RUN_TEST(test_constant_input_settles_to_its_square);
    RUN_TEST(test_unit_sine_settles_to_one_half);
    RUN_TEST(test_reset_returns_to_zero);
    return UNITY_END();
}
