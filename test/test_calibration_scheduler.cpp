#include "anc/calibration_scheduler.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

anc::CalibrationScheduler makeScheduler() {
    anc::CalibrationScheduler scheduler(600000, 30000);
    scheduler.markCalibrated(1000);
    return scheduler;
}

void test_drift_waits_for_cooldown() {
    const auto scheduler = makeScheduler();
    TEST_ASSERT_FALSE(scheduler.shouldRecheck(20000, true));
    TEST_ASSERT_TRUE(scheduler.shouldRecheck(40000, true));
}

void test_backstop_without_drift() {
    const auto scheduler = makeScheduler();
    TEST_ASSERT_FALSE(scheduler.shouldRecheck(500000, false));
    TEST_ASSERT_TRUE(scheduler.shouldRecheck(601000, false));
}

void test_survives_counter_wraparound() {
    anc::CalibrationScheduler scheduler(600000, 30000);
    scheduler.markCalibrated(0xFFFFF000u);
    // 0x00010000 is about 69.6 s after 0xFFFFF000 once the counter wraps.
    TEST_ASSERT_TRUE(scheduler.shouldRecheck(0x00010000u, true));
    TEST_ASSERT_FALSE(scheduler.shouldRecheck(0x00010000u, false));
}

void test_never_calibrated_means_now() {
    const anc::CalibrationScheduler scheduler(600000, 30000);
    TEST_ASSERT_TRUE(scheduler.shouldRecheck(0, false));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_drift_waits_for_cooldown);
    RUN_TEST(test_backstop_without_drift);
    RUN_TEST(test_survives_counter_wraparound);
    RUN_TEST(test_never_calibrated_means_now);
    return UNITY_END();
}
