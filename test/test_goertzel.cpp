#include <cmath>

#include "anc/goertzel.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr float kRate             = 8000.0f;
constexpr std::size_t kN          = 800;  // exactly six cycles of 60 Hz
constexpr std::size_t kPartCycleN = 850;  // 6.375 cycles of 60 Hz
constexpr double kTwoPi           = 6.283185307179586;
float samples[kPartCycleN];

// Adds amplitude * sin(2 pi f t + phase) to the first n samples.
void addSine(float freqHz, float amplitude, double phase, std::size_t n = kN) {
    for (std::size_t i = 0; i < n; ++i) {
        samples[i] += static_cast<float>(
            amplitude *
            std::sin(kTwoPi * freqHz * static_cast<double>(i) / kRate + phase));
    }
}

void clear() {
    for (float& s : samples)
        s = 0.0f;
}

void test_sine_measures_its_amplitude() {
    clear();
    addSine(60.0f, 0.5f, 0.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f,
                             anc::toneAmplitude(samples, kN, 60.0f, kRate));
}

void test_phase_offset_does_not_change_amplitude() {
    clear();
    addSine(60.0f, 0.5f, 1.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f,
                             anc::toneAmplitude(samples, kN, 60.0f, kRate));
}

void test_other_frequency_is_ignored() {
    clear();
    addSine(120.0f, 0.5f, 0.0);
    TEST_ASSERT_TRUE(anc::toneAmplitude(samples, kN, 60.0f, kRate) < 0.01f);
}

void test_tone_is_measured_alongside_another() {
    clear();
    addSine(60.0f, 0.5f, 0.0);
    addSine(120.0f, 0.3f, 0.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f,
                             anc::toneAmplitude(samples, kN, 60.0f, kRate));
}

void test_empty_block_is_zero() {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, anc::toneAmplitude(nullptr, 0, 60.0f, kRate));
}

// With 6.375 cycles in the block the reading is off by about 1% (0.506
// for 0.5), so "leak in a little" is small. Allow 10%.
void test_part_cycle_block_is_close() {
    clear();
    addSine(60.0f, 0.5f, 0.0, kPartCycleN);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.5f,
                             anc::toneAmplitude(samples, kPartCycleN, 60.0f, kRate));
}

void test_part_cycle_reading_is_linear() {
    clear();
    addSine(60.0f, 0.5f, 0.0, kPartCycleN);
    const float full = anc::toneAmplitude(samples, kPartCycleN, 60.0f, kRate);

    clear();
    addSine(60.0f, 0.25f, 0.0, kPartCycleN);
    const float half = anc::toneAmplitude(samples, kPartCycleN, 60.0f, kRate);

    TEST_ASSERT_FLOAT_WITHIN(0.01f * full / 2.0f, full / 2.0f, half);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sine_measures_its_amplitude);
    RUN_TEST(test_phase_offset_does_not_change_amplitude);
    RUN_TEST(test_other_frequency_is_ignored);
    RUN_TEST(test_tone_is_measured_alongside_another);
    RUN_TEST(test_empty_block_is_zero);
    RUN_TEST(test_part_cycle_block_is_close);
    RUN_TEST(test_part_cycle_reading_is_linear);
    return UNITY_END();
}
