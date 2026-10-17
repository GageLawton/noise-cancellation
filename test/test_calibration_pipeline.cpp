#include "anc/ir_analysis.hpp"
#include "anc/ir_estimator.hpp"
#include "anc/probe_processor.hpp"
#include "fakes/simulated_room.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr std::size_t kTaps    = 16;
constexpr std::size_t kSamples = 2400;  // a 300 ms probe at 8 kHz

// Runs the calibration pieces together, the way the firmware will: the
// probe plays into a simulated room, then the recording is turned back
// into the room's speaker-to-mic path.
void test_recovers_room_path() {
    // The "true" path: 5 samples of delay, then a peak and a small undershoot.
    float path[kTaps] = {};
    path[5]           = 0.6f;
    path[6]           = -0.2f;
    SimulatedRoom room(8000.0f, path, kTaps);  // no hum

    // ProbeProcessor holds 2 * 4096 floats, so keep it off the stack.
    static anc::ProbeProcessor probe;
    probe.begin(kSamples);

    // One sample at a time. The mic sample handed to processBlock is the
    // one the room produced on the previous step, just as the real
    // pipeline delays the mic by one step.
    float mic = 0.0f;
    float out = 0.0f;
    while (!probe.done()) {
        probe.processBlock(&mic, &out, 1);
        mic = room.step(out);
    }

    float h[kTaps];
    const auto& rec = probe.recording();
    anc::estimateImpulseResponse(rec.probe(), rec.mic(), rec.size(), h, kTaps);
    const anc::IrPeak peak = anc::findPeak(h, kTaps);

    // The room's peak is at tap 5, but the measured one lands at tap 6.
    // Each mic sample is recorded one step after the room produced it, so
    // the whole path looks one sample longer. The real firmware sees the
    // same extra sample, and the canceller needs to know about it, so
    // calibration measuring it is correct.
    TEST_ASSERT_EQUAL_size_t(6, peak.index);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.6f, peak.value);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, -0.2f, h[7]);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_recovers_room_path);
    return UNITY_END();
}
