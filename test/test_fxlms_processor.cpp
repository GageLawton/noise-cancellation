#include "anc/fxlms_processor.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

// Static: the canceller holds large path filters.
anc::FxlmsCanceller canc(8000.0f);

void test_silent_mic_gives_silent_output() {
    const float path[] = {1.0f};
    canc.setSecondaryPath(path, 1);
    canc.setNormalizedStepSize(0.1f);

    anc::FxlmsProcessor processor(canc);
    anc::IBlockProcessor& block = processor;

    const float mic[32] = {};
    for (int b = 0; b < 4; ++b) {
        float out[32];
        for (float& x : out) {
            x = 1.0f;
        }
        block.processBlock(mic, out, 32);
        for (float x : out) {
            TEST_ASSERT_EQUAL_FLOAT(0.0f, x);
        }
    }
    // Nothing to cancel, so the weights never moved.
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.weight(0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.weight(1));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_silent_mic_gives_silent_output);
    return UNITY_END();
}
