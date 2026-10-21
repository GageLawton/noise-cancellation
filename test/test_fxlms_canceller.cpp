#include <cmath>
#include <cstdio>

#include "anc/fxlms_canceller.hpp"
#include "anc/node_config.hpp"
#include "anc/ref_generator.hpp"
#include "fakes/simulated_room.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr float kFs = 8000.0f;

// Static: each canceller holds two 1024-tap filters.
anc::FxlmsCanceller canc(kFs);

void test_zero_weights_give_silence() {
    canc               = anc::FxlmsCanceller(kFs);
    const float path[] = {1.0f};
    canc.setSecondaryPath(path, 1);
    canc.setToneFrequency(0, 60.0f);
    for (int i = 0; i < 100; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.nextOutput());
    }
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.weight(0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.weight(1));
}

void test_unit_path_passes_reference_through() {
    canc               = anc::FxlmsCanceller(kFs);
    const float path[] = {1.0f};
    canc.setSecondaryPath(path, 1);
    canc.setToneFrequency(0, 60.0f);

    anc::RefGenerator ref(kFs);
    ref.setFrequency(60.0f);
    for (int i = 0; i < 100; ++i) {
        float s = 0.0f;
        float c = 0.0f;
        ref.next(s, c);
        canc.nextOutput();
        TEST_ASSERT_EQUAL_FLOAT(s, canc.filteredSin());
        TEST_ASSERT_EQUAL_FLOAT(c, canc.filteredCos());
    }
}

void test_one_sample_delay_path_gives_previous_reference() {
    canc               = anc::FxlmsCanceller(kFs);
    const float path[] = {0.0f, 1.0f};
    canc.setSecondaryPath(path, 2);
    canc.setToneFrequency(0, 60.0f);

    anc::RefGenerator ref(kFs);
    ref.setFrequency(60.0f);
    float prevSin = 0.0f;
    float prevCos = 0.0f;
    for (int i = 0; i < 100; ++i) {
        float s = 0.0f;
        float c = 0.0f;
        ref.next(s, c);
        canc.nextOutput();
        TEST_ASSERT_EQUAL_FLOAT(prevSin, canc.filteredSin());
        TEST_ASSERT_EQUAL_FLOAT(prevCos, canc.filteredCos());
        prevSin = s;
        prevCos = c;
    }
}

void test_cancels_simulated_hum() {
    float path[16] = {};
    path[5]        = 0.6f;
    path[6]        = -0.2f;

    canc = anc::FxlmsCanceller(kFs);
    canc.setSecondaryPath(path, 16);
    canc.setToneFrequency(0, 60.0f);
    canc.setNormalizedStepSize(0.1f);

    SimulatedRoom room(kFs, path, 16);
    room.addHum(60.0f, 0.5f, 0.7f);

    constexpr int kTotal = 16000;  // 2 s
    constexpr int kTail  = 2000;
    double tailSquares   = 0.0;
    for (int i = 0; i < kTotal; ++i) {
        const float y   = canc.nextOutput();
        const float mic = room.step(y);
        canc.update(mic);
        if (i >= kTotal - kTail) {
            tailSquares += static_cast<double>(mic) * mic;
        }
    }

    const float humRms      = 0.5f / std::sqrt(2.0f);
    const float residualRms = static_cast<float>(std::sqrt(tailSquares / kTail));
    const float reductionDb = 20.0f * std::log10(humRms / residualRms);
    std::printf("cancellation: %.1f dB\n", static_cast<double>(reductionDb));
    TEST_ASSERT_TRUE(reductionDb >= 40.0f);
}

void test_cancels_two_tones() {
    float path[16] = {};
    path[5]        = 0.6f;
    path[6]        = -0.2f;

    canc = anc::FxlmsCanceller(kFs);
    canc.setSecondaryPath(path, 16);
    canc.setNumTones(2);
    canc.setToneFrequency(0, 60.0f);
    canc.setToneFrequency(1, 120.0f);
    canc.setNormalizedStepSize(0.1f);

    SimulatedRoom room(kFs, path, 16);
    room.addHum(60.0f, 0.5f, 0.7f);
    room.addHum(120.0f, 0.3f, 0.0f);

    constexpr int kTotal = 16000;  // 2 s
    constexpr int kTail  = 2000;
    double tailSquares   = 0.0;
    for (int i = 0; i < kTotal; ++i) {
        const float mic = room.step(canc.nextOutput());
        canc.update(mic);
        if (i >= kTotal - kTail) {
            tailSquares += static_cast<double>(mic) * mic;
        }
    }

    // The two hums are uncorrelated, so their powers add.
    const float humRms      = std::sqrt((0.5f * 0.5f + 0.3f * 0.3f) / 2.0f);
    const float residualRms = static_cast<float>(std::sqrt(tailSquares / kTail));
    const float reductionDb = 20.0f * std::log10(humRms / residualRms);
    std::printf("two-tone cancellation: %.1f dB\n", static_cast<double>(reductionDb));
    TEST_ASSERT_TRUE(reductionDb >= 35.0f);
}

void test_reports_residual_and_convergence() {
    float path[16] = {};
    path[5]        = 0.6f;
    path[6]        = -0.2f;

    canc = anc::FxlmsCanceller(kFs);
    canc.setSecondaryPath(path, 16);
    canc.setNumTones(2);
    canc.setToneFrequency(0, 60.0f);
    canc.setToneFrequency(1, 120.0f);
    canc.setNormalizedStepSize(0.1f);
    TEST_ASSERT_FALSE(canc.converged());

    SimulatedRoom room(kFs, path, 16);
    room.addHum(60.0f, 0.5f, 0.7f);
    room.addHum(120.0f, 0.3f, 0.0f);

    bool convergedEarly = false;
    for (int i = 0; i < 16000; ++i) {
        canc.update(room.step(canc.nextOutput()));
        if (i == 100) {
            convergedEarly = canc.converged();
        }
    }
    TEST_ASSERT_FALSE(convergedEarly);
    TEST_ASSERT_TRUE(canc.converged());
    TEST_ASSERT_TRUE(canc.residualPower() < 1e-4f);
}

void test_loud_sample_resets_convergence() {
    canc = anc::FxlmsCanceller(kFs);
    canc.setConvergedThreshold(0.01f);
    for (std::size_t i = 0; i < anc::FxlmsCanceller::kConvergedSamples; ++i) {
        canc.update(0.0f);
    }
    TEST_ASSERT_TRUE(canc.converged());

    canc.update(10.0f);  // pushes the average well above 0.01
    TEST_ASSERT_FALSE(canc.converged());
}

void test_reset_forgets_learning() {
    float path[16] = {};
    path[5]        = 0.6f;
    path[6]        = -0.2f;

    canc = anc::FxlmsCanceller(kFs);
    canc.setSecondaryPath(path, 16);
    canc.setToneFrequency(0, 60.0f);
    canc.setNormalizedStepSize(0.1f);

    SimulatedRoom room(kFs, path, 16);
    room.addHum(60.0f, 0.5f, 0.7f);
    for (int i = 0; i < 16000; ++i) {
        canc.update(room.step(canc.nextOutput()));
    }
    TEST_ASSERT_TRUE(canc.converged());
    TEST_ASSERT_TRUE(std::fabs(canc.weight(0)) + std::fabs(canc.weight(1)) > 0.1f);

    canc.reset();
    TEST_ASSERT_FALSE(canc.converged());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.weight(0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.weight(1));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.residualPower());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.filteredSin());
    // The path and frequency are kept: it learns again from scratch.
    for (int i = 0; i < 16000; ++i) {
        canc.update(room.step(canc.nextOutput()));
    }
    TEST_ASSERT_TRUE(canc.converged());
}

// The firmware's own buffering puts the speaker-to-mic peak around 160
// samples plus the acoustic path (design doc 6.2). A delay that long
// lowers the step size FxLMS can take before it goes unstable: 0.1, the
// old default, diverges here. This runs the shipped defaults (taps and
// step size) through such a path, with a model that has only half the
// real gain, as a calibration might, and with a little mic noise.
void test_default_settings_cancel_through_firmware_sized_delay() {
    const anc::NodeConfig cfg = anc::NodeConfig::defaults();
    TEST_ASSERT_TRUE(cfg.secondaryPathTaps >= 200);

    static float truePath[1024]  = {};
    static float modelPath[1024] = {};
    const std::size_t taps       = cfg.secondaryPathTaps;
    const std::size_t delay      = 180;
    const float shape[]          = {0.4f, 0.24f, 0.0f, 0.0f, -0.1f};
    for (std::size_t k = 0; k < 5; ++k) {
        truePath[delay + k]  = shape[k];
        modelPath[delay + k] = 0.5f * shape[k];
    }

    const float freqsHz[] = {60.0f, 120.0f, 180.0f};
    for (const float freqHz : freqsHz) {
        canc = anc::FxlmsCanceller(kFs);
        canc.setSecondaryPath(modelPath, taps);
        canc.setToneFrequency(0, freqHz);
        canc.setNormalizedStepSize(cfg.normalizedStepSize);

        SimulatedRoom room(kFs, truePath, taps);
        room.addHum(freqHz, 0.3f, 1.1f);

        constexpr int kTotal = 40000;  // 5 s
        constexpr int kTail  = 4000;
        double tailSquares   = 0.0;
        uint32_t noise       = 1;
        for (int i = 0; i < kTotal; ++i) {
            noise ^= noise << 13;
            noise ^= noise >> 17;
            noise ^= noise << 5;
            const float hiss =
                0.003f * (static_cast<float>(noise >> 8) / 8388608.0f - 1.0f);
            const float mic = room.step(canc.nextOutput()) + hiss;
            canc.update(mic);
            if (i >= kTotal - kTail) {
                tailSquares += static_cast<double>(mic) * mic;
            }
        }

        const float humRms      = 0.3f / std::sqrt(2.0f);
        const float residualRms = static_cast<float>(std::sqrt(tailSquares / kTail));
        const float reductionDb = 20.0f * std::log10(humRms / residualRms);
        std::printf("%.0f Hz through a 180-sample delay: %.1f dB\n",
                    static_cast<double>(freqHz), static_cast<double>(reductionDb));
        TEST_ASSERT_TRUE(std::isfinite(reductionDb));
        TEST_ASSERT_TRUE(reductionDb >= 30.0f);
        TEST_ASSERT_TRUE(canc.converged());
    }
}

void test_new_canceller_has_zero_weight_norm() {
    canc = anc::FxlmsCanceller(kFs);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.weightNorm());
}

void test_weight_norm_grows_when_learning_and_resets() {
    float path[16] = {};
    path[5]        = 0.6f;
    path[6]        = -0.2f;

    canc = anc::FxlmsCanceller(kFs);
    canc.setSecondaryPath(path, 16);
    canc.setToneFrequency(0, 60.0f);
    canc.setNormalizedStepSize(0.1f);

    SimulatedRoom room(kFs, path, 16);
    room.addHum(60.0f, 0.5f, 0.7f);
    for (int i = 0; i < 16000; ++i) {
        canc.update(room.step(canc.nextOutput()));
    }
    TEST_ASSERT_TRUE(canc.weightNorm() > 0.0f);

    canc.reset();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, canc.weightNorm());
}

void test_num_tones_is_clamped() {
    canc = anc::FxlmsCanceller(kFs);
    canc.setNumTones(0);
    TEST_ASSERT_EQUAL_UINT(1, canc.numTones());
    canc.setNumTones(99);
    TEST_ASSERT_EQUAL_UINT(anc::FxlmsCanceller::kTones, canc.numTones());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_zero_weights_give_silence);
    RUN_TEST(test_unit_path_passes_reference_through);
    RUN_TEST(test_one_sample_delay_path_gives_previous_reference);
    RUN_TEST(test_cancels_simulated_hum);
    RUN_TEST(test_cancels_two_tones);
    RUN_TEST(test_num_tones_is_clamped);
    RUN_TEST(test_reports_residual_and_convergence);
    RUN_TEST(test_loud_sample_resets_convergence);
    RUN_TEST(test_reset_forgets_learning);
    RUN_TEST(test_default_settings_cancel_through_firmware_sized_delay);
    RUN_TEST(test_new_canceller_has_zero_weight_norm);
    RUN_TEST(test_weight_norm_grows_when_learning_and_resets);
    return UNITY_END();
}
