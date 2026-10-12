#include "anc/silence_processor.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_outputs_silence_for_the_entire_block() {
    anc::SilenceProcessor processor;
    float mic[32];
    float output[32];
    for (int i = 0; i < 32; ++i) {
        mic[i]    = 0.5f;
        output[i] = 1.0f;
    }

    anc::IBlockProcessor& interface = processor;
    interface.processBlock(mic, output, 32);
    for (int i = 0; i < 32; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(0.0f, output[i]);
        TEST_ASSERT_EQUAL_FLOAT(0.5f, mic[i]);
    }
}

void test_empty_block_does_not_touch_output() {
    anc::SilenceProcessor processor;
    float output = 1.0f;
    processor.processBlock(nullptr, &output, 0);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, output);
    processor.processBlock(nullptr, nullptr, 0);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_outputs_silence_for_the_entire_block);
    RUN_TEST(test_empty_block_does_not_touch_output);
    return UNITY_END();
}
