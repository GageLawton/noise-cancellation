#include "anc/node_config.hpp"
#include "unity.h"

using anc::NodeConfig;

void setUp() {}
void tearDown() {}

namespace {

void test_defaults_are_valid() {
    NodeConfig c = NodeConfig::defaults();
    TEST_ASSERT_TRUE(c.validate());
}

void test_defaults_match_design_doc() {
    NodeConfig c = NodeConfig::defaults();
    TEST_ASSERT_EQUAL_UINT16(128, c.secondaryPathTaps);
    TEST_ASSERT_EQUAL_FLOAT(0.1f, c.normalizedStepSize);
    TEST_ASSERT_EQUAL_UINT16(32, c.dmaBlockSamples);
    TEST_ASSERT_EQUAL_UINT8(1, c.numTones);
    TEST_ASSERT_EQUAL_FLOAT(60.0f, c.initialFreqHz[0]);
}

void test_rejects_step_size_outside_stability_bound() {
    NodeConfig c = NodeConfig::defaults();

    c.normalizedStepSize = 0.0f;
    TEST_ASSERT_FALSE(c.validate());

    c.normalizedStepSize = 2.0f;  // NLMS bound is strictly (0, 2)
    TEST_ASSERT_FALSE(c.validate());

    c.normalizedStepSize = 1.9f;
    TEST_ASSERT_TRUE(c.validate());
}

void test_rejects_tone_above_nyquist() {
    NodeConfig c = NodeConfig::defaults();

    c.initialFreqHz[0] = anc::kSampleRateHz / 2.0f;  // exactly Nyquist
    TEST_ASSERT_FALSE(c.validate());

    c.initialFreqHz[0] = 3999.0f;  // just under
    TEST_ASSERT_TRUE(c.validate());
}

void test_rejects_watch_threshold_above_trigger() {
    NodeConfig c = NodeConfig::defaults();

    // The watch gate must trip before the calibration trigger, or it
    // would never mark anything low-confidence.
    c.watchCorrelationThreshold = c.correlationThreshold;
    TEST_ASSERT_FALSE(c.validate());

    c.watchCorrelationThreshold = c.correlationThreshold - 0.05f;
    TEST_ASSERT_TRUE(c.validate());
}

void test_rejects_backstop_shorter_than_cooldown() {
    NodeConfig c         = NodeConfig::defaults();
    c.backstopIntervalMs = c.cooldownMs - 1;
    TEST_ASSERT_FALSE(c.validate());
}

void test_rejects_tone_count_out_of_range() {
    NodeConfig c = NodeConfig::defaults();

    c.numTones = 0;
    TEST_ASSERT_FALSE(c.validate());

    c.numTones = anc::kMaxTones + 1;
    TEST_ASSERT_FALSE(c.validate());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_defaults_are_valid);
    RUN_TEST(test_defaults_match_design_doc);
    RUN_TEST(test_rejects_step_size_outside_stability_bound);
    RUN_TEST(test_rejects_tone_above_nyquist);
    RUN_TEST(test_rejects_watch_threshold_above_trigger);
    RUN_TEST(test_rejects_backstop_shorter_than_cooldown);
    RUN_TEST(test_rejects_tone_count_out_of_range);
    return UNITY_END();
}
