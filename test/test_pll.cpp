#include <cmath>

#include "anc/pll.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr double kTwoPi = 6.283185307179586;
constexpr float kFs     = 8000.0f;

float tone(double freqHz, float amplitude, int n) {
    return amplitude * static_cast<float>(std::sin(kTwoPi * freqHz * n / kFs));
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

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_pulls_in_from_58_to_60);
    RUN_TEST(test_stays_on_frequency);
    return UNITY_END();
}
