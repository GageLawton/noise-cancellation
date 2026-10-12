#include "anc/gain_ramp.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_starts_silent() {
    anc::GainRamp ramp(400);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ramp.gain());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ramp.process(1.0f));
}

void test_fades_in_over_the_ramp_length() {
    anc::GainRamp ramp(400);
    ramp.setTarget(1.0f);

    TEST_ASSERT_EQUAL_FLOAT(0.0f, ramp.process(1.0f));  // sample 0
    for (int i = 1; i < 200; ++i) {
        ramp.process(1.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, ramp.process(1.0f));  // sample 200
    for (int i = 201; i < 400; ++i) {
        ramp.process(1.0f);
    }
    for (int i = 400; i < 500; ++i) {
        TEST_ASSERT_TRUE(ramp.process(1.0f) == 1.0f);  // exactly, not nearly
    }
}

void test_fades_back_out_to_exactly_zero() {
    anc::GainRamp ramp(400);
    ramp.setTarget(1.0f);
    for (int i = 0; i < 400; ++i) {
        ramp.process(1.0f);
    }

    ramp.setTarget(0.0f);
    for (int i = 0; i < 400; ++i) {
        ramp.process(1.0f);
    }
    TEST_ASSERT_TRUE(ramp.gain() == 0.0f);
    TEST_ASSERT_TRUE(ramp.process(1.0f) == 0.0f);
}

void test_never_overshoots_the_target() {
    anc::GainRamp ramp(3);  // 1/3 steps don't add up to exactly 1 in float
    ramp.setTarget(1.0f);
    for (int i = 0; i < 10; ++i) {
        ramp.process(1.0f);
        TEST_ASSERT_TRUE(ramp.gain() <= 1.0f);
    }
    TEST_ASSERT_TRUE(ramp.gain() == 1.0f);

    ramp.setTarget(0.0f);
    for (int i = 0; i < 10; ++i) {
        ramp.process(1.0f);
        TEST_ASSERT_TRUE(ramp.gain() >= 0.0f);
    }
    TEST_ASSERT_TRUE(ramp.gain() == 0.0f);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_starts_silent);
    RUN_TEST(test_fades_in_over_the_ramp_length);
    RUN_TEST(test_fades_back_out_to_exactly_zero);
    RUN_TEST(test_never_overshoots_the_target);
    return UNITY_END();
}
