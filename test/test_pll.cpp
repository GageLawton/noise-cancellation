#include <cmath>

#include "anc/pll.hpp"
#include "anc/probe_generator.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr double kTwoPi = 6.283185307179586;
constexpr float kFs     = 8000.0f;

float tone(double freqHz, float amplitude, int n) {
    return amplitude * static_cast<float>(std::sin(kTwoPi * freqHz * n / kFs));
}

void test_pulls_in_from_48_to_50() {
    anc::Pll pll(48.0f, kFs);
    for (int n = 0; n < 3 * 8000; ++n) {
        pll.update(tone(50.0f, 0.5f, n));
    }
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 50.0f, pll.frequencyHz());
}

void test_locks_onto_clean_50hz_tone() {
    anc::Pll pll(50.0f, kFs);
    for (int n = 0; n < 2 * 8000; ++n) {
        pll.update(tone(50.0f, 0.5f, n));
    }
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 50.0f, pll.frequencyHz());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, pll.phaseError());
}

void test_pulls_in_from_58_to_60() {
    anc::Pll pll(58.0f, kFs);
    for (int n = 0; n < 3 * 8000; ++n) {
        pll.update(tone(60.0, 0.5f, n));
    }
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 60.0f, pll.frequencyHz());
}

void test_stays_on_frequency() {
    anc::Pll pll(60.0f, kFs);
    for (int n = 0; n < 2 * 8000; ++n) {
        pll.update(tone(60.0, 0.5f, n));
    }
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 60.0f, pll.frequencyHz());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, pll.phaseError());
}

void test_follows_slow_drift() {
    anc::Pll pll(60.0f, kFs);
    pll.setMaxDriftRate(2.0f);

    // Sweep 60 -> 61 Hz over 2 s by accumulating phase.
    double phase = 0.0;
    for (int n = 0; n < 2 * 8000; ++n) {
        const float freq = 60.0f + static_cast<float>(n) / (2.0f * kFs);
        pll.update(0.5f * static_cast<float>(std::sin(phase)));
        phase += kTwoPi * freq / kFs;
    }
    TEST_ASSERT_FLOAT_WITHIN(0.15f, 61.0f, pll.frequencyHz());
}

void test_ignores_sudden_jump() {
    anc::Pll pll(60.0f, kFs);
    pll.setMaxDriftRate(2.0f);

    double phase      = 0.0;
    float biggestStep = 0.0f;
    for (int n = 0; n < 2 * 8000; ++n) {
        const double freq  = n < 8000 ? 60.0 : 70.0;
        const float before = pll.frequencyHz();
        pll.update(0.5f * static_cast<float>(std::sin(phase)));
        biggestStep = std::fmax(biggestStep, std::fabs(pll.frequencyHz() - before));
        phase += kTwoPi * freq / kFs;
    }
    // Allow for float rounding when adding a tiny step to ~60.
    TEST_ASSERT_TRUE(biggestStep <= 2.0f / kFs + 1e-5f);
    TEST_ASSERT_TRUE(pll.frequencyHz() <= 62.0f);
}

void test_locks_onto_clean_tone() {
    anc::Pll pll(60.0f, kFs);
    TEST_ASSERT_FALSE(pll.locked());
    for (int n = 0; n < 2 * 8000; ++n) {
        pll.update(tone(60.0, 0.5f, n));
    }
    TEST_ASSERT_TRUE(pll.locked());
}

void test_does_not_lock_on_noise() {
    anc::Pll pll(60.0f, kFs);
    anc::ProbeGenerator noise(3, 0.3f);
    for (int n = 0; n < 2 * 8000; ++n) {
        pll.update(noise.next());
    }
    TEST_ASSERT_FALSE(pll.locked());
}

void test_stays_locked_with_noise_on_top() {
    anc::Pll pll(60.0f, kFs);
    anc::ProbeGenerator noise(3, 0.2f);
    for (int n = 0; n < 3 * 8000; ++n) {
        pll.update(tone(60.0, 0.5f, n) + noise.next());
    }
    TEST_ASSERT_TRUE(pll.locked());
}

void test_losing_the_tone_unlocks() {
    anc::Pll pll(60.0f, kFs);
    for (int n = 0; n < 2 * 8000; ++n) {
        pll.update(tone(60.0, 0.5f, n));
    }
    for (int n = 0; n < 8000; ++n) {
        pll.update(0.0f);
    }
    TEST_ASSERT_FALSE(pll.locked());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_pulls_in_from_48_to_50);
    RUN_TEST(test_locks_onto_clean_50hz_tone);
    RUN_TEST(test_pulls_in_from_58_to_60);
    RUN_TEST(test_stays_on_frequency);
    RUN_TEST(test_follows_slow_drift);
    RUN_TEST(test_ignores_sudden_jump);
    RUN_TEST(test_locks_onto_clean_tone);
    RUN_TEST(test_does_not_lock_on_noise);
    RUN_TEST(test_stays_locked_with_noise_on_top);
    RUN_TEST(test_losing_the_tone_unlocks);
    return UNITY_END();
}
