#include <algorithm>
#include <cmath>
#include <vector>

#include "anc/decimator2x.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr float kTwoPi      = 6.28318530718f;
constexpr float kInputRate  = 16000.0f;
constexpr std::size_t kSize = 3200;

std::vector<float> sine(float freq) {
    std::vector<float> samples(kSize);
    for (std::size_t i = 0; i < kSize; ++i) {
        samples[i] = std::sin(kTwoPi * freq * static_cast<float>(i) / kInputRate);
    }
    return samples;
}

// Largest absolute value, skipping the first 20 outputs while the filter
// fills up.
float peakAfterSettling(const std::vector<float>& samples, std::size_t count) {
    float peak = 0.0f;
    for (std::size_t i = 20; i < count; ++i) {
        peak = std::max(peak, std::abs(samples[i]));
    }
    return peak;
}

void test_keeps_a_low_tone() {
    anc::Decimator2x decimator;
    const std::vector<float> input = sine(100.0f);
    std::vector<float> output(kSize / 2);
    const std::size_t written = decimator.process(input.data(), kSize, output.data());
    TEST_ASSERT_EQUAL_UINT(1600, written);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, peakAfterSettling(output, written));
}

void test_blocks_a_tone_above_4_khz() {
    anc::Decimator2x decimator;
    const std::vector<float> input = sine(7000.0f);
    std::vector<float> output(kSize / 2);
    const std::size_t written = decimator.process(input.data(), kSize, output.data());
    TEST_ASSERT_TRUE(peakAfterSettling(output, written) < 0.1f);
}

void test_split_blocks_match_one_block() {
    const std::vector<float> input = sine(100.0f);

    anc::Decimator2x whole;
    std::vector<float> expected(kSize / 2);
    whole.process(input.data(), kSize, expected.data());

    // Odd-sized pieces check that the keep/drop phase carries over.
    anc::Decimator2x pieces;
    std::vector<float> actual(kSize / 2);
    std::size_t written = 0;
    std::size_t start   = 0;
    for (std::size_t piece : {7u, 33u, 160u}) {
        while (start + piece <= kSize) {
            written +=
                pieces.process(input.data() + start, piece, actual.data() + written);
            start += piece;
        }
    }
    written +=
        pieces.process(input.data() + start, kSize - start, actual.data() + written);

    TEST_ASSERT_EQUAL_UINT(kSize / 2, written);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected.data(), actual.data(), kSize / 2);
}

void test_reset_clears_state() {
    anc::Decimator2x decimator;
    const float loud[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float out[2];
    decimator.process(loud, 4, out);
    decimator.reset();

    const float quiet[2] = {0.0f, 0.0f};
    TEST_ASSERT_EQUAL_UINT(1, decimator.process(quiet, 2, out));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out[0]);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_keeps_a_low_tone);
    RUN_TEST(test_blocks_a_tone_above_4_khz);
    RUN_TEST(test_split_blocks_match_one_block);
    RUN_TEST(test_reset_clears_state);
    return UNITY_END();
}
