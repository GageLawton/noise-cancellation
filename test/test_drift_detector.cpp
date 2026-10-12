#include "anc/drift_detector.hpp"
#include "anc/probe_generator.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr int kFourSeconds = 32000;

void test_unrelated_signals_raise_no_flags() {
    anc::DriftDetector drift(0.3f, 0.15f, 8000, 8000);
    anc::ProbeGenerator noise(1, 1.0f);
    anc::ProbeGenerator synth(2, 1.0f);
    for (int n = 0; n < kFourSeconds; ++n) {
        drift.update(noise.next(), synth.next());
    }
    TEST_ASSERT_FALSE(drift.lowConfidence());
    TEST_ASSERT_FALSE(drift.needsCalibration());
}

void test_leftover_speaker_requests_calibration() {
    anc::DriftDetector drift(0.3f, 0.15f, 8000, 8000);
    anc::ProbeGenerator synth(1, 1.0f);
    anc::ProbeGenerator room(2, 1.0f);
    for (int n = 0; n < kFourSeconds; ++n) {
        const float y = synth.next();
        // Correlation of about 0.5 / sqrt(1.25), roughly 0.45.
        drift.update(0.5f * y + room.next(), y);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.45f, drift.correlation());
    TEST_ASSERT_TRUE(drift.lowConfidence());
    TEST_ASSERT_TRUE(drift.needsCalibration());
}

void test_short_burst_is_not_enough() {
    anc::DriftDetector drift(0.3f, 0.15f, 100, 8000);
    anc::ProbeGenerator synth(1, 1.0f);
    for (int n = 0; n < 4000; ++n) {  // half the sustain time
        const float y = synth.next();
        drift.update(y, y);
    }
    TEST_ASSERT_TRUE(drift.lowConfidence());
    TEST_ASSERT_FALSE(drift.needsCalibration());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_unrelated_signals_raise_no_flags);
    RUN_TEST(test_leftover_speaker_requests_calibration);
    RUN_TEST(test_short_burst_is_not_enough);
    return UNITY_END();
}
