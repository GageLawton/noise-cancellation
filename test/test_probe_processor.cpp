#include "anc/probe_processor.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr int kBlock = 32;

// Static: the 4096-sample recorder is too big to put on the stack lightly.
anc::ProbeProcessor processor;

void test_plays_probe_then_silence() {
    processor.begin(64);
    TEST_ASSERT_FALSE(processor.done());

    float mic[3][kBlock];
    float out[3][kBlock];
    for (int b = 0; b < 3; ++b) {
        for (int i = 0; i < kBlock; ++i) {
            mic[b][i] = 0.001f * (b * kBlock + i);
        }
    }

    processor.processBlock(mic[0], out[0], kBlock);
    TEST_ASSERT_FALSE(processor.done());
    processor.processBlock(mic[1], out[1], kBlock);
    TEST_ASSERT_TRUE(processor.done());
    processor.processBlock(mic[2], out[2], kBlock);

    for (int i = 0; i < kBlock; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(0.0f, out[2][i]);
    }

    const auto& rec = processor.recording();
    TEST_ASSERT_EQUAL_UINT(64, rec.size());
    for (int b = 0; b < 2; ++b) {
        for (int i = 0; i < kBlock; ++i) {
            TEST_ASSERT_EQUAL_FLOAT(mic[b][i], rec.mic()[b * kBlock + i]);
            TEST_ASSERT_EQUAL_FLOAT(out[b][i], rec.probe()[b * kBlock + i]);
        }
    }
}

void test_begin_replays_the_same_probe() {
    float mic[kBlock] = {};
    float first[kBlock];
    float second[kBlock];

    processor.begin(kBlock);
    processor.processBlock(mic, first, kBlock);
    processor.begin(kBlock);
    processor.processBlock(mic, second, kBlock);

    for (int i = 0; i < kBlock; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(first[i], second[i]);
    }
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_plays_probe_then_silence);
    RUN_TEST(test_begin_replays_the_same_probe);
    return UNITY_END();
}
