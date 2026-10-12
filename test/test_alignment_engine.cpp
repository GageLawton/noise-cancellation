#include <cmath>

#include "anc/alignment_engine.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr double kTwoPi = 6.283185307179586;

float tone60(int n) {
    return 0.5f * static_cast<float>(std::sin(kTwoPi * 60.0 * n / anc::kSampleRateHz));
}

// Static: the engine holds a 1024-tap path filter.
anc::AlignmentEngine engine;

void test_tracks_the_room_hum() {
    anc::NodeConfig cfg  = anc::NodeConfig::defaults();
    cfg.initialFreqHz[0] = 59.0f;
    const float path[]   = {1.0f};
    engine.configure(cfg, path, 1);
    TEST_ASSERT_EQUAL_UINT(1, engine.numTones());

    for (int n = 0; n < 3 * 8000; ++n) {
        engine.update(tone60(n), 0.0f);  // our speaker is silent
    }
    TEST_ASSERT_TRUE(engine.locked(0));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 60.0f, engine.frequencyHz(0));
    TEST_ASSERT_FALSE(engine.needsCalibration());
}

void test_stale_path_requests_calibration() {
    const anc::NodeConfig cfg = anc::NodeConfig::defaults();
    const float path[]        = {1.0f};
    engine.configure(cfg, path, 1);

    bool requested = false;
    for (int n = 0; n < 3 * 8000 && !requested; ++n) {
        const float synth = tone60(n);
        // The real path is 1.5x what the engine was told, so half of our
        // own output is left in the estimated noise.
        engine.update(1.5f * synth, synth);
        requested = engine.needsCalibration();
    }
    TEST_ASSERT_TRUE(requested);
    TEST_ASSERT_TRUE(engine.lowConfidence());
    TEST_ASSERT_TRUE(engine.correlation() > 0.9f);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_tracks_the_room_hum);
    RUN_TEST(test_stale_path_requests_calibration);
    return UNITY_END();
}
