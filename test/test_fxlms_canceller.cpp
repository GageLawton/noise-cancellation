#include "anc/fxlms_canceller.hpp"
#include "anc/ref_generator.hpp"
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
    canc.setToneFrequency(60.0f);
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
    canc.setToneFrequency(60.0f);

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
    canc.setToneFrequency(60.0f);

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

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_zero_weights_give_silence);
    RUN_TEST(test_unit_path_passes_reference_through);
    RUN_TEST(test_one_sample_delay_path_gives_previous_reference);
    return UNITY_END();
}
