#include "anc/tone_processor.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

// 1000 Hz at 8000 Hz is one cycle every 8 samples, amplitude 0.5.
const float kExpected[8] = {0.0f, 0.354f, 0.5f, 0.354f, 0.0f, -0.354f, -0.5f, -0.354f};

void test_plays_a_sine_at_the_requested_frequency_and_level() {
    anc::ToneProcessor tone(8000.0f, 1000.0f, 0.5f);
    float out[8] = {};

    anc::IBlockProcessor& interface = tone;
    interface.processBlock(nullptr, out, 8);
    for (int i = 0; i < 8; ++i) {
        TEST_ASSERT_FLOAT_WITHIN(0.001f, kExpected[i], out[i]);
    }
}

void test_phase_carries_over_between_blocks() {
    anc::ToneProcessor tone(8000.0f, 1000.0f, 0.5f);
    float out[8] = {};

    tone.processBlock(nullptr, out, 4);
    tone.processBlock(nullptr, out + 4, 4);
    for (int i = 0; i < 8; ++i) {
        TEST_ASSERT_FLOAT_WITHIN(0.001f, kExpected[i], out[i]);
    }
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_plays_a_sine_at_the_requested_frequency_and_level);
    RUN_TEST(test_phase_carries_over_between_blocks);
    return UNITY_END();
}
