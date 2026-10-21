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
    TEST_ASSERT_EQUAL_UINT16(256, c.secondaryPathTaps);
    TEST_ASSERT_EQUAL_FLOAT(0.005f, c.normalizedStepSize);
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

void test_rejects_non_positive_tone() {
    NodeConfig c = NodeConfig::defaults();

    c.initialFreqHz[0] = 0.0f;
    TEST_ASSERT_FALSE(c.validate());

    c.initialFreqHz[0] = -60.0f;
    TEST_ASSERT_FALSE(c.validate());
}

void test_ignores_unused_tone_slots() {
    NodeConfig c = NodeConfig::defaults();

    c.numTones         = 1;
    c.initialFreqHz[1] = 0.0f;
    TEST_ASSERT_TRUE(c.validate());

    c.numTones         = 2;
    c.initialFreqHz[1] = 120.0f;
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

void test_rejects_correlation_threshold_out_of_range() {
    // correlationThreshold must be strictly between 0 and 1.
    NodeConfig c = NodeConfig::defaults();

    c.correlationThreshold = 0.0f;
    TEST_ASSERT_FALSE(c.validate());

    c.correlationThreshold = 1.0f;
    TEST_ASSERT_FALSE(c.validate());

    c.correlationThreshold = 0.5f;
    TEST_ASSERT_TRUE(c.validate());
}

void test_rejects_non_positive_drift_rate() {
    // maxDriftRateHzPerSec must be greater than 0. Check that 0.0 and -1.0 are
    // rejected.
    NodeConfig c = NodeConfig::defaults();

    c.maxDriftRateHzPerSec = 0.0f;
    TEST_ASSERT_FALSE(c.validate());

    c.maxDriftRateHzPerSec = -1.0f;
    TEST_ASSERT_FALSE(c.validate());

    c.maxDriftRateHzPerSec = 1.0f;
    TEST_ASSERT_TRUE(c.validate());
}

void test_min_lock_amplitude_default_is_valid() {
    NodeConfig c = NodeConfig::defaults();
    TEST_ASSERT_EQUAL_FLOAT(0.1f, c.minLockAmplitude);
    TEST_ASSERT_TRUE(c.validate());
}

void test_rejects_min_lock_amplitude_out_of_range() {
    NodeConfig c = NodeConfig::defaults();

    c.minLockAmplitude = 0.0f;
    TEST_ASSERT_FALSE(c.validate());
    c.minLockAmplitude = 1.5f;
    TEST_ASSERT_FALSE(c.validate());
    c.minLockAmplitude = 1.0f;
    TEST_ASSERT_TRUE(c.validate());
}

void test_rejects_dma_block_size_out_of_range() {
    // validate range for dmaBlockSamples is 8 to 256 inclusive.
    // Check 7 and 257 are rejected. And 8 and 256 are accepted

    NodeConfig c = NodeConfig::defaults();

    c.dmaBlockSamples = 7;
    TEST_ASSERT_FALSE(c.validate());

    c.dmaBlockSamples = 257;
    TEST_ASSERT_FALSE(c.validate());

    c.dmaBlockSamples = 8;
    TEST_ASSERT_TRUE(c.validate());

    c.dmaBlockSamples = 256;
    TEST_ASSERT_TRUE(c.validate());
}

void test_rejects_empty_node_name() {
    NodeConfig c  = NodeConfig::defaults();
    c.nodeName[0] = '\0';
    TEST_ASSERT_FALSE(c.validate());
}

void test_rejects_secondary_path_taps_out_of_range() {
    NodeConfig c = NodeConfig::defaults();

    c.secondaryPathTaps = 7;
    TEST_ASSERT_FALSE(c.validate());
    c.secondaryPathTaps = 1025;
    TEST_ASSERT_FALSE(c.validate());
    c.secondaryPathTaps = 8;
    TEST_ASSERT_TRUE(c.validate());
    c.secondaryPathTaps = 1024;
    TEST_ASSERT_TRUE(c.validate());
}

void test_rejects_calibration_timing_out_of_range() {
    NodeConfig c = NodeConfig::defaults();

    c.probeDurationMs = 49;
    TEST_ASSERT_FALSE(c.validate());
    c.probeDurationMs = 5001;
    TEST_ASSERT_FALSE(c.validate());
    c.probeDurationMs = 50;
    TEST_ASSERT_TRUE(c.validate());
    c.probeDurationMs = 5000;
    TEST_ASSERT_TRUE(c.validate());

    c                     = NodeConfig::defaults();
    c.correlationWindowMs = 99;
    TEST_ASSERT_FALSE(c.validate());
    c.correlationWindowMs = 10001;
    TEST_ASSERT_FALSE(c.validate());
    c.correlationWindowMs = 100;
    TEST_ASSERT_TRUE(c.validate());
    c.correlationWindowMs = 10000;
    TEST_ASSERT_TRUE(c.validate());
}

void test_output_defaults() {
    NodeConfig c = NodeConfig::defaults();
    TEST_ASSERT_EQUAL_FLOAT(0.8f, c.outputCeiling);
    TEST_ASSERT_EQUAL_FLOAT(440.0f, c.testToneHz);
    TEST_ASSERT_EQUAL_FLOAT(0.1f, c.testToneAmplitude);
}

void test_rejects_output_ceiling_out_of_range() {
    NodeConfig c = NodeConfig::defaults();

    c.outputCeiling = 0.0f;
    TEST_ASSERT_FALSE(c.validate());

    c.outputCeiling = 1.01f;
    TEST_ASSERT_FALSE(c.validate());

    c.outputCeiling = 1.0f;
    TEST_ASSERT_TRUE(c.validate());
}

void test_rejects_test_tone_out_of_range() {
    NodeConfig c = NodeConfig::defaults();

    c.testToneHz = anc::kSampleRateHz / 2.0f;  // exactly Nyquist
    TEST_ASSERT_FALSE(c.validate());

    c.testToneHz = 0.0f;
    TEST_ASSERT_FALSE(c.validate());

    c.testToneHz = 3999.0f;
    TEST_ASSERT_TRUE(c.validate());
}

void test_rejects_test_tone_louder_than_ceiling() {
    NodeConfig c = NodeConfig::defaults();

    c.testToneAmplitude = c.outputCeiling + 0.01f;
    TEST_ASSERT_FALSE(c.validate());

    c.testToneAmplitude = 0.0f;
    TEST_ASSERT_FALSE(c.validate());

    c.testToneAmplitude = c.outputCeiling;
    TEST_ASSERT_TRUE(c.validate());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_defaults_are_valid);
    RUN_TEST(test_defaults_match_design_doc);
    RUN_TEST(test_rejects_step_size_outside_stability_bound);
    RUN_TEST(test_rejects_tone_above_nyquist);
    RUN_TEST(test_rejects_non_positive_tone);
    RUN_TEST(test_ignores_unused_tone_slots);
    RUN_TEST(test_rejects_watch_threshold_above_trigger);
    RUN_TEST(test_rejects_backstop_shorter_than_cooldown);
    RUN_TEST(test_rejects_tone_count_out_of_range);
    RUN_TEST(test_rejects_correlation_threshold_out_of_range);
    RUN_TEST(test_rejects_non_positive_drift_rate);
    RUN_TEST(test_min_lock_amplitude_default_is_valid);
    RUN_TEST(test_rejects_min_lock_amplitude_out_of_range);
    RUN_TEST(test_rejects_dma_block_size_out_of_range);
    RUN_TEST(test_rejects_empty_node_name);
    RUN_TEST(test_rejects_secondary_path_taps_out_of_range);
    RUN_TEST(test_rejects_calibration_timing_out_of_range);
    RUN_TEST(test_output_defaults);
    RUN_TEST(test_rejects_output_ceiling_out_of_range);
    RUN_TEST(test_rejects_test_tone_out_of_range);
    RUN_TEST(test_rejects_test_tone_louder_than_ceiling);
    return UNITY_END();
}
