#include "anc/passthrough_processor.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_unity_gain_copies_the_block_exactly() {
    anc::PassthroughProcessor processor(1.0f);
    float mic[8] = {0.0f, 0.25f, -0.5f, 1.0f, -1.0f, 0.125f, -0.75f, 0.3f};
    float out[8] = {};

    anc::IBlockProcessor& interface = processor;
    interface.processBlock(mic, out, 8);
    for (int i = 0; i < 8; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(mic[i], out[i]);
    }
}

void test_half_gain_halves_every_sample() {
    anc::PassthroughProcessor processor(0.5f);
    float mic[8] = {0.0f, 0.25f, -0.5f, 1.0f, -1.0f, 0.125f, -0.75f, 0.3f};
    float out[8] = {};

    processor.processBlock(mic, out, 8);
    for (int i = 0; i < 8; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(0.5f * mic[i], out[i]);
    }
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_unity_gain_copies_the_block_exactly);
    RUN_TEST(test_half_gain_halves_every_sample);
    return UNITY_END();
}
