#include "anc/i2s_settings.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_defaults_match_audio_configuration() {
    const anc::I2sSettings settings = anc::defaultI2sSettings();
    TEST_ASSERT_EQUAL_INT(8000, settings.sampleRateHz);
    TEST_ASSERT_EQUAL_INT(4, settings.bclkPin);
    TEST_ASSERT_EQUAL_INT(5, settings.wsPin);
    TEST_ASSERT_EQUAL_INT(6, settings.doutPin);
    TEST_ASSERT_EQUAL_INT(7, settings.dinPin);
    TEST_ASSERT_EQUAL_INT(32, settings.dmaFrames);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_defaults_match_audio_configuration);
    return UNITY_END();
}
