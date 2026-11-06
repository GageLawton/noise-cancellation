#include <cmath>
#include <cstdio>

#include "anc/alignment_engine.hpp"
#include "anc/fxlms_canceller.hpp"
#include "anc/goertzel.hpp"
#include "fakes/simulated_room.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr double kTwoPi       = 6.283185307179586;
constexpr float kFs           = anc::kSampleRateHz;
constexpr std::size_t kTaps   = 16;
constexpr int kTotal          = 80000;  // 10 s: the whole glide
constexpr std::size_t kWindow = 800;    // the last 100 ms
constexpr int kBlock          = 32;     // follow the tracker this often
constexpr double kStartHz     = 60.0;
constexpr double kEndHz       = 60.5;
constexpr float kHumAmplitude = 0.3f;

// The hum's frequency at sample n: a straight glide from kStartHz to
// kEndHz over the whole run.
double humHz(int n) {
    return kStartHz + (kEndHz - kStartHz) * n / kTotal;
}

// Static: the canceller and the engine each hold 1024-tap filters, and
// the mic log 10 s of samples.
anc::FxlmsCanceller canc(kFs);
anc::AlignmentEngine engine;
float micLog[kTotal];

// As in the firmware's filterLoop(): the canceller follows the hum the
// AlignmentEngine tracks, a block at a time. The room is given no hum of
// its own; the drifting hum is added here, and both the canceller and
// the engine get the true path, so there is no calibration step.
void test_cancel_hum_that_drifts() {
    float path[kTaps] = {};
    path[5]           = 0.6f;
    path[6]           = -0.2f;
    SimulatedRoom room(kFs, path, kTaps);

    canc = anc::FxlmsCanceller(kFs);
    canc.setSecondaryPath(path, kTaps);
    canc.setToneFrequency(0, static_cast<float>(kStartHz));
    canc.setNormalizedStepSize(0.1f);

    // Configured as in test_alignment_engine, with the true path.
    anc::NodeConfig cfg  = anc::NodeConfig::defaults();
    cfg.initialFreqHz[0] = static_cast<float>(kStartHz);
    engine.configure(cfg, path, kTaps);

    // Accumulate the hum's phase sample by sample so it doesn't jump as
    // the frequency changes.
    double phase = 0.0;
    for (int i = 0; i < kTotal; ++i) {
        if (i % kBlock == 0 && engine.locked(0)) {
            canc.setToneFrequency(0, engine.frequencyHz(0));
        }
        const float hum = kHumAmplitude * static_cast<float>(std::sin(phase));
        phase += kTwoPi * humHz(i) / kFs;

        const float out = canc.nextOutput();
        const float mic = room.step(out) + hum;
        canc.update(mic);
        engine.update(mic, out);  // the mic and the speaker sample played with it
        micLog[i] = mic;
    }

    // Measure at the hum's frequency in the middle of the last window.
    const float nowHz =
        static_cast<float>(humHz(kTotal - static_cast<int>(kWindow) / 2));
    const float after =
        anc::toneAmplitude(micLog + kTotal - kWindow, kWindow, nowHz, kFs);
    const float dropDb = 20.0f * std::log10(kHumAmplitude / after);
    std::printf("%.2f Hz at the mic: %.4f -> %.6f (%.1f dB), tracker at %.3f Hz%s\n",
                static_cast<double>(nowHz), static_cast<double>(kHumAmplitude),
                static_cast<double>(after), static_cast<double>(dropDb),
                static_cast<double>(engine.frequencyHz(0)),
                engine.locked(0) ? " (locked)" : "");
    TEST_ASSERT_TRUE(std::isfinite(dropDb));
    // The issue asks for at least 15 dB, but the canceller's weights alone
    // reach about 42 dB on this slow glide even with frequency following
    // turned off (about 80 dB with it). 60 dB sits between the two, so this
    // fails if the tracker's frequency stops reaching the canceller.
    TEST_ASSERT_TRUE(dropDb >= 60.0f);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_cancel_hum_that_drifts);
    return UNITY_END();
}
