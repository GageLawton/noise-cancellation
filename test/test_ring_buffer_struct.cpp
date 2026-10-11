#include "anc/spsc_ring_buffer.hpp"
#include "unity.h"

using anc::SpscRingBuffer;

void setUp() {}
void tearDown() {}

namespace {

struct SamplePair {
    float mic;
    float synth;
};

void test_struct_values_survive_round_trip() {
    SpscRingBuffer<SamplePair, 8> rb;
    const SamplePair expected[] = {
        {0.25f, -0.5f},
        {-1.0f, 0.75f},
        {3.5f, 2.0f},
    };

    for (const auto& sample : expected) {
        TEST_ASSERT_TRUE(rb.push(sample));
    }

    for (const auto& sample : expected) {
        SamplePair actual{};
        TEST_ASSERT_TRUE(rb.pop(actual));
        TEST_ASSERT_EQUAL_FLOAT(sample.mic, actual.mic);
        TEST_ASSERT_EQUAL_FLOAT(sample.synth, actual.synth);
    }
}

void test_struct_buffer_reports_full() {
    SpscRingBuffer<SamplePair, 4> rb;
    const SamplePair first{1.0f, -1.0f};
    const SamplePair second{2.0f, -2.0f};
    const SamplePair third{3.0f, -3.0f};
    const SamplePair fourth{4.0f, -4.0f};

    TEST_ASSERT_TRUE(rb.push(first));
    TEST_ASSERT_TRUE(rb.push(second));
    TEST_ASSERT_TRUE(rb.push(third));
    TEST_ASSERT_FALSE(rb.push(fourth));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_struct_values_survive_round_trip);
    RUN_TEST(test_struct_buffer_reports_full);
    return UNITY_END();
}
