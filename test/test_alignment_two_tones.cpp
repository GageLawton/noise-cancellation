#include <cmath>

#include "anc/alignment_engine.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr double kTwoPi = 6.283185307179586;

// A 60 Hz hum with its 120 Hz harmonic at half the level.
float humWithHarmonic(int n) {
    const double t = static_cast<double>(n) / anc::kSampleRateHz;
    return static_cast<float>(0.5 * std::sin(kTwoPi * 60.0 * t) +
                              0.25 * std::sin(kTwoPi * 120.0 * t));
}

// Static: the engine holds a 1024-tap path filter.
anc::AlignmentEngine engine;

void test_locks_hum_and_harmonic_together() {
    anc::NodeConfig cfg  = anc::NodeConfig::defaults();
    cfg.numTones         = 2;
    cfg.initialFreqHz[0] = 59.0f;
    cfg.initialFreqHz[1] = 119.0f;
    const float path[]   = {1.0f};
    engine.configure(cfg, path, 1);
    TEST_ASSERT_EQUAL_UINT(2, engine.numTones());

    for (int n = 0; n < 3 * 8000; ++n) {
        engine.update(humWithHarmonic(n), 0.0f);  // our speaker is silent
    }
    // Each PLL stays on its own tone instead of being pulled by the other.
    TEST_ASSERT_TRUE(engine.locked(0));
    TEST_ASSERT_TRUE(engine.locked(1));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 60.0f, engine.frequencyHz(0));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 120.0f, engine.frequencyHz(1));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_locks_hum_and_harmonic_together);
    return UNITY_END();
}
