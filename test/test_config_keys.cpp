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
    TEST_ASSERT_EQUAL_FLOAT(anc::NodeConfig::defaults().normalizedStepSize,
                            cfg.normalizedStepSize);
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

void test_formats_default_step_size() {
    const anc::NodeConfig cfg = anc::NodeConfig::defaults();
    char out[16];
    TEST_ASSERT_TRUE(
        anc::formatSetting(cfg, "dsp.normalized_step_size", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("0.005", out);
}

void test_formats_default_block_size() {
    const anc::NodeConfig cfg = anc::NodeConfig::defaults();
    char out[16];
    TEST_ASSERT_TRUE(
        anc::formatSetting(cfg, "dsp.dma_block_samples", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("32", out);
    TEST_ASSERT_TRUE(
        anc::formatSetting(cfg, "calibration.backstop_interval_ms", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("600000", out);
}

void test_format_unknown_key_is_false() {
    const anc::NodeConfig cfg = anc::NodeConfig::defaults();
    char out[16];
    TEST_ASSERT_FALSE(anc::formatSetting(cfg, "dsp.nope", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("", out);
}

void test_format_too_small_buffer_is_false() {
    const anc::NodeConfig cfg = anc::NodeConfig::defaults();
    char out[3];
    TEST_ASSERT_FALSE(
        anc::formatSetting(cfg, "dsp.normalized_step_size", out, sizeof(out)));
}

void test_set_then_get_round_trips() {
    anc::NodeConfig cfg = anc::NodeConfig::defaults();
    char out[16];
    TEST_ASSERT_TRUE(anc::applySetting(cfg, "dsp.normalized_step_size", "0.25"));
    TEST_ASSERT_TRUE(
        anc::formatSetting(cfg, "dsp.normalized_step_size", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("0.250", out);
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
    RUN_TEST(test_formats_default_step_size);
    RUN_TEST(test_formats_default_block_size);
    RUN_TEST(test_format_unknown_key_is_false);
    RUN_TEST(test_format_too_small_buffer_is_false);
    RUN_TEST(test_set_then_get_round_trips);
    return UNITY_END();
}
