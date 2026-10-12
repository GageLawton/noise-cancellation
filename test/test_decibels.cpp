#include "anc/decibels.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_full_amplitude_is_zero_db() {
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, anc::amplitudeToDb(1.0f));
}

void test_half_amplitude_is_about_minus_six_db() {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -6.02f, anc::amplitudeToDb(0.5f));
}

void test_silence_and_small_amplitudes_use_reporting_floor() {
    TEST_ASSERT_EQUAL_FLOAT(-120.0f, anc::amplitudeToDb(0.0f));
    TEST_ASSERT_EQUAL_FLOAT(-120.0f, anc::amplitudeToDb(1e-6f));
    TEST_ASSERT_EQUAL_FLOAT(-120.0f, anc::amplitudeToDb(1e-7f));
    TEST_ASSERT_EQUAL_FLOAT(-120.0f, anc::amplitudeToDb(-1.0f));
}

void test_minus_six_db_is_about_half_amplitude() {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, anc::dbToAmplitude(-6.02f));
}

void test_amplitude_round_trip() {
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.25f,
                             anc::dbToAmplitude(anc::amplitudeToDb(0.25f)));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_full_amplitude_is_zero_db);
    RUN_TEST(test_half_amplitude_is_about_minus_six_db);
    RUN_TEST(test_silence_and_small_amplitudes_use_reporting_floor);
    RUN_TEST(test_minus_six_db_is_about_half_amplitude);
    RUN_TEST(test_amplitude_round_trip);
    return UNITY_END();
}
