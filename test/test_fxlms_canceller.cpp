#include <cmath>
#include <cstdio>

#include "anc/fxlms_canceller.hpp"
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
    return UNITY_END();
}
