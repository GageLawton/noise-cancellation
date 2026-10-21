#include "anc/ir_analysis.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_finds_largest_absolute_tap_and_keeps_sign() {
    const float h[]        = {0.0f, 0.1f, -0.7f, 0.3f};
    const anc::IrPeak peak = anc::findPeak(h, 4);
    TEST_ASSERT_EQUAL_UINT(2, peak.index);
    TEST_ASSERT_EQUAL_FLOAT(-0.7f, peak.value);
}

void test_all_zero_gives_index_zero() {
    const float h[]        = {0.0f, 0.0f, 0.0f};
    const anc::IrPeak peak = anc::findPeak(h, 3);
    TEST_ASSERT_EQUAL_UINT(0, peak.index);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, peak.value);
}

void test_delay_in_milliseconds() {
    TEST_ASSERT_EQUAL_FLOAT(5.0f, anc::delayMs(40, 8000.0f));
}

void test_taps_for_energy_single_spike() {
    const float h[] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    TEST_ASSERT_EQUAL_UINT(3, anc::tapsForEnergy(h, 5, 0.99f));
}

void test_taps_for_energy_flat_response() {
    const float h[] = {1.0f, 1.0f, 1.0f, 1.0f};
    TEST_ASSERT_EQUAL_UINT(2, anc::tapsForEnergy(h, 4, 0.5f));
    TEST_ASSERT_EQUAL_UINT(4, anc::tapsForEnergy(h, 4, 1.0f));
}

void test_taps_for_energy_all_zero_gives_zero() {
    const float h[] = {0.0f, 0.0f, 0.0f};
    TEST_ASSERT_EQUAL_UINT(0, anc::tapsForEnergy(h, 3, 0.99f));
    TEST_ASSERT_EQUAL_UINT(0, anc::tapsForEnergy(h, 0, 0.99f));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_finds_largest_absolute_tap_and_keeps_sign);
    RUN_TEST(test_all_zero_gives_index_zero);
    RUN_TEST(test_delay_in_milliseconds);
    RUN_TEST(test_taps_for_energy_single_spike);
    RUN_TEST(test_taps_for_energy_flat_response);
    RUN_TEST(test_taps_for_energy_all_zero_gives_zero);
    return UNITY_END();
}
