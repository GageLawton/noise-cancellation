#include "anc/clock_offset.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr uint64_t kStartUs = 1000000000ull;
constexpr uint64_t kStepUs  = 1000000ull;

void addPairs(anc::ClockOffset& clock, uint64_t count, uint64_t offsetUs) {
    for (uint64_t i = 0; i < count; ++i) {
        const uint64_t local = kStartUs + i * kStepUs;
        clock.add(local, local + offsetUs);
    }
}

void test_reset_is_not_ready() {
    anc::ClockOffset clock;
    addPairs(clock, 5, 1000);
    TEST_ASSERT_TRUE(clock.ready());
    clock.reset();
    TEST_ASSERT_FALSE(clock.ready());
}

void test_reset_forgets_the_old_offset() {
    anc::ClockOffset clock;
    addPairs(clock, 5, 50000);
    clock.reset();
    addPairs(clock, 4, 1000);
    TEST_ASSERT_TRUE(clock.ready());
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 1000.0f, static_cast<float>(clock.offsetUs()));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, static_cast<float>(clock.driftPpm()));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_reset_is_not_ready);
    RUN_TEST(test_reset_forgets_the_old_offset);
    return UNITY_END();
}
