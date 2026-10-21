#include <cmath>

#include "anc/clock_offset.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

// Unity's double asserts are off in this build, so results are checked
// as float; the tolerances are far wider than float's rounding here.

// One pair a second, starting a while after boot so the clocks are big.
constexpr uint64_t kStartUs = 1000000000ull;
constexpr uint64_t kStepUs  = 1000000ull;

void test_constant_offset_has_no_drift() {
    anc::ClockOffset clock;
    for (uint64_t i = 0; i < 20; ++i) {
        const uint64_t local = kStartUs + i * kStepUs;
        clock.add(local, local + 5000);
    }
    TEST_ASSERT_TRUE(clock.ready());
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 5000.0f, static_cast<float>(clock.offsetUs()));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, static_cast<float>(clock.driftPpm()));
}

void test_fast_remote_clock_shows_its_drift() {
    anc::ClockOffset clock;
    for (uint64_t i = 0; i < 20; ++i) {
        const uint64_t local = kStartUs + i * kStepUs;
        const uint64_t remote =
            static_cast<uint64_t>(std::llround(local * 1.00002 + 300.0));
        clock.add(local, remote);
    }
    TEST_ASSERT_TRUE(clock.ready());
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 20.0f, static_cast<float>(clock.driftPpm()));
}

void test_not_ready_with_fewer_than_four_pairs() {
    anc::ClockOffset clock;
    TEST_ASSERT_FALSE(clock.ready());
    for (uint64_t i = 0; i < 3; ++i) {
        clock.add(kStartUs + i * kStepUs, kStartUs + i * kStepUs + 5000);
        TEST_ASSERT_FALSE(clock.ready());
    }
    clock.add(kStartUs + 3 * kStepUs, kStartUs + 3 * kStepUs + 5000);
    TEST_ASSERT_TRUE(clock.ready());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_constant_offset_has_no_drift);
    RUN_TEST(test_fast_remote_clock_shows_its_drift);
    RUN_TEST(test_not_ready_with_fewer_than_four_pairs);
    return UNITY_END();
}
