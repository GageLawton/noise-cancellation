#include "anc/config_keys.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_sets_step_size() {
    anc::NodeConfig cfg = anc::NodeConfig::defaults();
    TEST_ASSERT_TRUE(anc::applySetting(cfg, "dsp.normalized_step_size", "0.2"));
    TEST_ASSERT_EQUAL_FLOAT(0.2f, cfg.normalizedStepSize);
}

void test_invalid_value_keeps_old_one() {
    anc::NodeConfig cfg = anc::NodeConfig::defaults();
    TEST_ASSERT_FALSE(anc::applySetting(cfg, "dsp.normalized_step_size", "2.5"));
    TEST_ASSERT_EQUAL_FLOAT(0.1f, cfg.normalizedStepSize);
}

void test_unknown_key_is_rejected() {
    anc::NodeConfig cfg = anc::NodeConfig::defaults();
    TEST_ASSERT_FALSE(anc::applySetting(cfg, "dsp.nope", "1"));
    TEST_ASSERT_FALSE(anc::applySetting(cfg, "identity.node_name", "kitchen"));
}

void test_unparsable_values_are_rejected() {
    anc::NodeConfig cfg = anc::NodeConfig::defaults();
    TEST_ASSERT_FALSE(anc::applySetting(cfg, "dsp.dma_block_samples", "abc"));
    TEST_ASSERT_FALSE(anc::applySetting(cfg, "dsp.dma_block_samples", "32x"));
    TEST_ASSERT_FALSE(anc::applySetting(cfg, "dsp.dma_block_samples", ""));
    TEST_ASSERT_FALSE(anc::applySetting(cfg, "dsp.dma_block_samples", "-1"));
    TEST_ASSERT_FALSE(anc::applySetting(cfg, "dsp.dma_block_samples", "70000"));
    TEST_ASSERT_FALSE(anc::applySetting(cfg, "dsp.normalized_step_size", "0.2.1"));
    TEST_ASSERT_FALSE(anc::applySetting(cfg, "dsp.normalized_step_size", "nan"));
    TEST_ASSERT_EQUAL_UINT16(32, cfg.dmaBlockSamples);
}

void test_sets_output_ceiling() {
    anc::NodeConfig cfg   = anc::NodeConfig::defaults();
    cfg.testToneAmplitude = 0.1f;  // must stay under the new ceiling
    TEST_ASSERT_TRUE(anc::applySetting(cfg, "output.ceiling", "0.5"));
    TEST_ASSERT_EQUAL_FLOAT(0.5f, cfg.outputCeiling);
}

void test_sets_integer_fields() {
    anc::NodeConfig cfg = anc::NodeConfig::defaults();
    TEST_ASSERT_TRUE(anc::applySetting(cfg, "dsp.dma_block_samples", "64"));
    TEST_ASSERT_EQUAL_UINT16(64, cfg.dmaBlockSamples);
    TEST_ASSERT_TRUE(anc::applySetting(cfg, "calibration.cooldown_ms", "45000"));
    TEST_ASSERT_EQUAL_UINT32(45000, cfg.cooldownMs);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sets_step_size);
    RUN_TEST(test_invalid_value_keeps_old_one);
    RUN_TEST(test_unknown_key_is_rejected);
    RUN_TEST(test_unparsable_values_are_rejected);
    RUN_TEST(test_sets_output_ceiling);
    RUN_TEST(test_sets_integer_fields);
    return UNITY_END();
}
