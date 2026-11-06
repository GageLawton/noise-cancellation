#include "anc/clock_offset.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr uint64_t kStartUs = 1000000000ull;
constexpr uint64_t kStepUs  = 1000000ull;

void test_remote_clock_jumping_back_starts_over() {
    anc::ClockOffset clock;
    uint64_t local = kStartUs;
    // The remote board has been up a long time.
    for (int i = 0; i < 5; ++i, local += kStepUs) {
        clock.add(local, local + 1000000000ull);
    }
    // It reboots, so its clock is small again; ours keeps growing.
    for (int i = 0; i < 4; ++i, local += kStepUs) {
        clock.add(local, local - 900000000ull);
    }
    TEST_ASSERT_TRUE(clock.ready());
    // Floats can't hold 9e8 to 1 us, so check the difference instead.
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, static_cast<float>(clock.offsetUs() + 900000000.0));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_remote_clock_jumping_back_starts_over);
    return UNITY_END();
}
