#include <algorithm>
#include <cmath>
#include <vector>

#include "anc/upsampler2x.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr float kTwoPi      = 6.28318530718f;
constexpr float kInputRate  = 8000.0f;
constexpr std::size_t kSize = 800;

std::vector<float> sine(float freq) {
    std::vector<float> samples(kSize);
    for (std::size_t i = 0; i < kSize; ++i) {
        samples[i] = std::sin(kTwoPi * freq * static_cast<float>(i) / kInputRate);
    }
    return samples;
}

void test_doubles_the_rate_of_a_low_tone() {
    anc::Upsampler2x upsampler;
    const std::vector<float> input = sine(100.0f);
    std::vector<float> output(2 * kSize);
    const std::size_t written = upsampler.process(input.data(), kSize, output.data());
    TEST_ASSERT_EQUAL_UINT(1600, written);

    // Skip the first 20 outputs while the filter fills up.
    float peak = 0.0f;
    for (std::size_t i = 20; i < written; ++i) {
        peak = std::max(peak, std::abs(output[i]));
    }
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, peak);
}

void test_split_blocks_match_one_block() {
    const std::vector<float> input = sine(100.0f);

    anc::Upsampler2x whole;
    std::vector<float> expected(2 * kSize);
    whole.process(input.data(), kSize, expected.data());

    anc::Upsampler2x pieces;
    std::vector<float> actual(2 * kSize);
    for (std::size_t start = 0; start < kSize; start += 160) {
        pieces.process(input.data() + start, 160, actual.data() + 2 * start);
    }
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected.data(), actual.data(), 2 * kSize);
}

void test_reset_clears_state() {
    anc::Upsampler2x upsampler;
    const float loud[2] = {1.0f, 1.0f};
    float out[4];
    upsampler.process(loud, 2, out);
    upsampler.reset();

    const float quiet[1] = {0.0f};
    upsampler.process(quiet, 1, out);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out[0]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out[1]);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_doubles_the_rate_of_a_low_tone);
    RUN_TEST(test_split_blocks_match_one_block);
    RUN_TEST(test_reset_clears_state);
    return UNITY_END();
}
