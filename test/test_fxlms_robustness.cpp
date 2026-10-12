#include <cmath>
#include <cstdio>

#include "anc/fxlms_canceller.hpp"
#include "fakes/simulated_room.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr float kFs     = 8000.0f;
constexpr int kTaps     = 16;
constexpr float kHumAmp = 0.5f;

// Static: each canceller holds two 1024-tap filters.
anc::FxlmsCanceller canc(kFs);

// Runs the 60 Hz room for 2 s with the true path [5] = 0.6, [6] = -0.2
// and the canceller using `estimate`. Returns the reduction in dB over
// the last 2000 samples.
float runRoom(const float* estimate, float mu) {
    float truePath[kTaps] = {};
    truePath[5]           = 0.6f;
    truePath[6]           = -0.2f;

    canc = anc::FxlmsCanceller(kFs);
    canc.setSecondaryPath(estimate, kTaps);
    canc.setToneFrequency(0, 60.0f);
    canc.setNormalizedStepSize(mu);

    SimulatedRoom room(kFs, truePath, kTaps);
    room.addHum(60.0f, kHumAmp, 0.7f);

    constexpr int kTotal = 16000;
    constexpr int kTail  = 2000;
    double tailSquares   = 0.0;
    for (int i = 0; i < kTotal; ++i) {
        const float mic = room.step(canc.nextOutput());
        canc.update(mic);
        if (i >= kTotal - kTail) {
            tailSquares += static_cast<double>(mic) * mic;
        }
    }
    const float humRms      = kHumAmp / std::sqrt(2.0f);
    const float residualRms = static_cast<float>(std::sqrt(tailSquares / kTail));
    return 20.0f * std::log10(humRms / residualRms);
}

void test_tolerates_gain_and_delay_error() {
    // 10% too loud and one sample late.
    float wrongPath[kTaps] = {};
    wrongPath[6]           = 0.66f;
    wrongPath[7]           = -0.22f;

    const float reductionDb = runRoom(wrongPath, 0.1f);
    std::printf("cancellation with a wrong path: %.1f dB\n",
                static_cast<double>(reductionDb));
    TEST_ASSERT_TRUE(reductionDb >= 30.0f);
}

void test_zero_step_size_never_adapts() {
    const float path[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, -0.2f};
    SimulatedRoom room(kFs, path, 7);
    room.addHum(60.0f, kHumAmp, 0.7f);

    canc = anc::FxlmsCanceller(kFs);
    canc.setSecondaryPath(path, 7);
    canc.setToneFrequency(0, 60.0f);
    canc.setNormalizedStepSize(0.0f);
    for (int i = 0; i < 1000; ++i) {
        canc.update(room.step(canc.nextOutput()));
    }
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.weight(0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.weight(1));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_tolerates_gain_and_delay_error);
    RUN_TEST(test_zero_step_size_never_adapts);
    return UNITY_END();
}
