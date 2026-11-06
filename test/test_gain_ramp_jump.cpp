#include "anc/gain_ramp.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_jump_cuts_a_fade_in_halfway() {
    anc::GainRamp ramp(400);
    ramp.setTarget(1.0f);
    for (int i = 0; i < 200; ++i) {
        ramp.process(1.0f);
    }

    ramp.jumpTo(0.0f);
    TEST_ASSERT_TRUE(ramp.done());
    TEST_ASSERT_TRUE(ramp.process(1.0f) == 0.0f);  // exactly, not nearly
}

void test_jump_holds_the_new_gain() {
    anc::GainRamp ramp(400);
    ramp.jumpTo(0.7f);
    for (int i = 0; i < 10; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(0.7f, ramp.process(1.0f));
    }
}

void test_fades_normally_after_a_jump() {
    anc::GainRamp ramp(400);
    ramp.jumpTo(1.0f);
    ramp.setTarget(0.0f);

    TEST_ASSERT_EQUAL_FLOAT(1.0f, ramp.process(1.0f));  // sample 0
    for (int i = 1; i < 200; ++i) {
        ramp.process(1.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, ramp.process(1.0f));  // sample 200
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_jump_cuts_a_fade_in_halfway);
    RUN_TEST(test_jump_holds_the_new_gain);
    RUN_TEST(test_fades_normally_after_a_jump);
    return UNITY_END();
}
