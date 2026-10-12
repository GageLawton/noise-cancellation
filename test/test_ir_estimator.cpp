#include "anc/fir_filter.hpp"
#include "anc/ir_estimator.hpp"
#include "anc/probe_generator.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr int kSamples = 2400;  // a 300 ms probe at 8 kHz
constexpr int kTaps    = 16;

void test_recovers_known_path() {
    float probe[kSamples];
    anc::ProbeGenerator gen(42, 0.5f);
    for (float& x : probe) {
        x = gen.next();
    }

    // The "true" path: 5 samples of delay, then a peak and a small undershoot.
    float truePath[kTaps] = {};
    truePath[5]           = 0.6f;
    truePath[6]           = -0.2f;
    anc::FirFilter<kTaps> path;
    path.setCoefficients(truePath, kTaps);

    float recorded[kSamples];
    for (int i = 0; i < kSamples; ++i) {
        recorded[i] = path.process(probe[i]);
    }

    float h[kTaps];
    anc::estimateImpulseResponse(probe, recorded, kSamples, h, kTaps);
    for (int k = 0; k < kTaps; ++k) {
        TEST_ASSERT_FLOAT_WITHIN(0.05f, truePath[k], h[k]);
    }
}

void test_silent_probe_gives_zero_taps() {
    const float probe[4]    = {};
    const float recorded[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float h[2]              = {9.0f, 9.0f};
    anc::estimateImpulseResponse(probe, recorded, 4, h, 2);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, h[0]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, h[1]);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_recovers_known_path);
    RUN_TEST(test_silent_probe_gives_zero_taps);
    return UNITY_END();
}
