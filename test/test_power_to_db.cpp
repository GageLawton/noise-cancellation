#include "anc/decibels.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_unit_power_is_zero_db() {
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, anc::powerToDb(1.0f));
}

// A full-scale sine has a mean square of 0.5.
void test_full_scale_sine_power_is_about_minus_three_db() {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -3.01f, anc::powerToDb(0.5f));
}

void test_silence_uses_reporting_floor() {
    TEST_ASSERT_EQUAL_FLOAT(-120.0f, anc::powerToDb(0.0f));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_unit_power_is_zero_db);
    RUN_TEST(test_full_scale_sine_power_is_about_minus_three_db);
    RUN_TEST(test_silence_uses_reporting_floor);
    return UNITY_END();
}
