#include "anc/divergence_detector.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_steady_power_never_trips() {
    anc::DivergenceDetector detector(0.25f, 25);
    for (int i = 0; i < 1000; ++i) {
        detector.update(0.01f);
    }
    TEST_ASSERT_FALSE(detector.diverged());
}

void test_one_block_over_hard_limit_trips() {
    anc::DivergenceDetector detector(0.25f, 25);
    detector.update(0.01f);
    detector.update(0.3f);
    TEST_ASSERT_TRUE(detector.diverged());

    // Latched: quiet blocks don't clear it.
    detector.update(0.01f);
    TEST_ASSERT_TRUE(detector.diverged());
}

void test_sustained_growth_trips() {
    anc::DivergenceDetector detector(0.25f, 25);
    float power = 1e-4f;
    for (int block = 1; block <= 24; ++block) {
        detector.update(power);
        power *= 1.1f;
    }
    TEST_ASSERT_FALSE(detector.diverged());

    detector.update(power);
    power *= 1.1f;
    detector.update(power);
    TEST_ASSERT_TRUE(detector.diverged());
}

void test_growth_that_pauses_starts_over() {
    anc::DivergenceDetector detector(0.25f, 25);
    // Five bursts of 20 growing blocks; each burst starts quiet again.
    for (int burst = 0; burst < 5; ++burst) {
        float power = 1e-4f;
        for (int block = 0; block < 20; ++block) {
            detector.update(power);
            power *= 1.1f;
        }
    }
    TEST_ASSERT_FALSE(detector.diverged());
}

void test_reset_clears_it() {
    anc::DivergenceDetector detector(0.25f, 25);
    detector.update(0.3f);
    detector.reset();
    TEST_ASSERT_FALSE(detector.diverged());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_steady_power_never_trips);
    RUN_TEST(test_one_block_over_hard_limit_trips);
    RUN_TEST(test_sustained_growth_trips);
    RUN_TEST(test_growth_that_pauses_starts_over);
    RUN_TEST(test_reset_clears_it);
    return UNITY_END();
}
