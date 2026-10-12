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

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_finds_largest_absolute_tap_and_keeps_sign);
    RUN_TEST(test_all_zero_gives_index_zero);
    RUN_TEST(test_delay_in_milliseconds);
    return UNITY_END();
}
