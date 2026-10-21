#include <cmath>

#include "anc/probe_processor.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr int kBlock = 32;

// Static: the 4096-sample recorder is too big to put on the stack lightly.
anc::ProbeProcessor processor;
anc::ProbeProcessor changed;
anc::ProbeProcessor unchanged;

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

void test_set_amplitude_applies_at_next_begin() {
    float mic[kBlock * 4] = {};
    float out[kBlock * 4];

    changed.setAmplitude(0.2f);
    changed.begin(kBlock * 4);
    changed.processBlock(mic, out, kBlock * 4);

    bool aboveOld = false;
    for (float s : out) {
        TEST_ASSERT_TRUE(std::fabs(s) <= 0.2f);
        if (std::fabs(s) > 0.1f) {
            aboveOld = true;
        }
    }
    TEST_ASSERT_TRUE(aboveOld);
}

void test_set_amplitude_mid_run_keeps_that_run() {
    float mic[kBlock] = {};
    float a[2][kBlock];
    float b[2][kBlock];

    // Same default amplitude and seed; only `changed` gets a new level,
    // and only after its run has started.
    changed.setAmplitude(0.5f);
    changed.begin(kBlock * 2);
    unchanged.begin(kBlock * 2);

    changed.processBlock(mic, a[0], kBlock);
    unchanged.processBlock(mic, b[0], kBlock);
    changed.setAmplitude(0.05f);
    changed.processBlock(mic, a[1], kBlock);
    unchanged.processBlock(mic, b[1], kBlock);

    for (int blk = 0; blk < 2; ++blk) {
        for (int i = 0; i < kBlock; ++i) {
            TEST_ASSERT_EQUAL_FLOAT(b[blk][i], a[blk][i]);
        }
    }
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_plays_probe_then_silence);
    RUN_TEST(test_begin_replays_the_same_probe);
    RUN_TEST(test_set_amplitude_applies_at_next_begin);
    RUN_TEST(test_set_amplitude_mid_run_keeps_that_run);
    return UNITY_END();
}
