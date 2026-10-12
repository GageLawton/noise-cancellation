#include "anc/sample_format.hpp"
#include "unity.h"

#include <cstdint>

void setUp() {}
void tearDown() {}

namespace {

void test_zero_sample_is_zero() {
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, anc::micSampleToFloat(0));
}

void test_minimum_sample_is_minus_one() {
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, -1.0f, anc::micSampleToFloat(INT32_MIN));
}

void test_maximum_sample_is_just_below_one() {
    const float sample = anc::micSampleToFloat(0x7FFFFF00);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.99999988f, sample);
}

void test_unused_low_bits_are_ignored() {
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, anc::micSampleToFloat(0x000000FF));
}

void test_speaker_zero_is_zero() {
    TEST_ASSERT_EQUAL_INT16(0, anc::floatToSpeakerSample(0.0f));
}

void test_speaker_endpoints_preserve_sign() {
    TEST_ASSERT_EQUAL_INT16(32767, anc::floatToSpeakerSample(1.0f));
    TEST_ASSERT_EQUAL_INT16(-32767, anc::floatToSpeakerSample(-1.0f));
}

void test_speaker_out_of_range_samples_are_clamped() {
    TEST_ASSERT_EQUAL_INT16(32767, anc::floatToSpeakerSample(2.0f));
    TEST_ASSERT_EQUAL_INT16(-32767, anc::floatToSpeakerSample(-5.0f));
}

void test_speaker_half_amplitude_is_rounded() {
    TEST_ASSERT_EQUAL_INT16(16384, anc::floatToSpeakerSample(0.5f));
    TEST_ASSERT_EQUAL_INT16(-16384, anc::floatToSpeakerSample(-0.5f));
}

void test_amp_zero_is_zero() {
    TEST_ASSERT_EQUAL_INT32(0, anc::floatToAmpSample(0.0f));
}

void test_amp_full_scale_endpoints_preserve_sign() {
    TEST_ASSERT_EQUAL_INT32(INT32_MAX, anc::floatToAmpSample(1.0f));
    TEST_ASSERT_EQUAL_INT32(-INT32_MAX, anc::floatToAmpSample(-1.0f));
}

void test_amp_out_of_range_samples_are_clamped() {
    TEST_ASSERT_EQUAL_INT32(INT32_MAX, anc::floatToAmpSample(1.5f));
    TEST_ASSERT_EQUAL_INT32(-INT32_MAX, anc::floatToAmpSample(-1.5f));
}

void test_amp_half_amplitude_is_rounded() {
    TEST_ASSERT_INT32_WITHIN(1, 1073741824, anc::floatToAmpSample(0.5f));
    TEST_ASSERT_INT32_WITHIN(1, -1073741824, anc::floatToAmpSample(-0.5f));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_zero_sample_is_zero);
    RUN_TEST(test_minimum_sample_is_minus_one);
    RUN_TEST(test_maximum_sample_is_just_below_one);
    RUN_TEST(test_unused_low_bits_are_ignored);
    RUN_TEST(test_speaker_zero_is_zero);
    RUN_TEST(test_speaker_endpoints_preserve_sign);
    RUN_TEST(test_speaker_out_of_range_samples_are_clamped);
    RUN_TEST(test_speaker_half_amplitude_is_rounded);
    RUN_TEST(test_amp_zero_is_zero);
    RUN_TEST(test_amp_full_scale_endpoints_preserve_sign);
    RUN_TEST(test_amp_out_of_range_samples_are_clamped);
    RUN_TEST(test_amp_half_amplitude_is_rounded);
    return UNITY_END();
}
