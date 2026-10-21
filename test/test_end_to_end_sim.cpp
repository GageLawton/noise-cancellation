#include <cmath>
#include <cstdio>

#include "anc/fxlms_canceller.hpp"
#include "anc/goertzel.hpp"
#include "anc/ir_estimator.hpp"
#include "anc/probe_processor.hpp"
#include "fakes/simulated_room.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr float kFs           = 8000.0f;
constexpr std::size_t kTaps   = 16;
constexpr std::size_t kProbe  = 2400;   // a 300 ms probe, as in calibration
constexpr int kTotal          = 40000;  // 5 s of cancelling
constexpr std::size_t kWindow = 800;    // 100 ms: six whole cycles of 60 Hz
constexpr float kHumHz        = 60.0f;

// Static: the probe holds 2 * 4096 floats, the canceller two 1024-tap
// filters, and the mic log 5 s of samples.
anc::ProbeProcessor probe;
anc::FxlmsCanceller canc(kFs);
float micLog[kTotal];

// The whole node in order, as the firmware runs it: calibrate the room
// with the hum present, then cancel the hum using the path that was
// measured rather than the true one.
void test_calibrate_then_cancel_hum() {
    float path[kTaps] = {};
    path[5]           = 0.6f;
    path[6]           = -0.2f;
    SimulatedRoom room(kFs, path, kTaps);
    room.addHum(kHumHz, 0.3f, 0.0f);

    // Calibrate, one sample at a time. As in test_calibration_pipeline,
    // the mic sample handed over is the one the room produced on the
    // previous step, just as the real pipeline delays the mic by one step.
    probe.begin(kProbe);
    float mic = 0.0f;
    float out = 0.0f;
    while (!probe.done()) {
        probe.processBlock(&mic, &out, 1);
        mic = room.step(out);
    }
    float estimated[kTaps];
    const auto& rec = probe.recording();
    anc::estimateImpulseResponse(rec.probe(), rec.mic(), rec.size(), estimated, kTaps);

    // Cancel with the measured path, settings as in test_fxlms_canceller.
    canc = anc::FxlmsCanceller(kFs);
    canc.setSecondaryPath(estimated, kTaps);
    canc.setToneFrequency(0, kHumHz);
    canc.setNormalizedStepSize(0.1f);

    // Same order as FxlmsProcessor, with the same one-step mic delay the
    // calibration measured.
    for (int i = 0; i < kTotal; ++i) {
        out = canc.nextOutput();
        canc.update(mic);
        mic       = room.step(out);
        micLog[i] = mic;
    }

    const float before = anc::toneAmplitude(micLog, kWindow, kHumHz, kFs);
    const float after =
        anc::toneAmplitude(micLog + kTotal - kWindow, kWindow, kHumHz, kFs);
    const float dropDb = 20.0f * std::log10(before / after);
    std::printf("60 Hz at the mic: %.4f -> %.6f (%.1f dB)\n",
                static_cast<double>(before), static_cast<double>(after),
                static_cast<double>(dropDb));
    TEST_ASSERT_TRUE(std::isfinite(dropDb));
    TEST_ASSERT_TRUE(dropDb >= 20.0f);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_calibrate_then_cancel_hum);
    return UNITY_END();
}
