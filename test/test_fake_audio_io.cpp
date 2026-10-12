#include "fakes/fake_audio_io.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_reads_remaining_microphone_samples() {
    FakeAudioIo audio({0.1f, -0.2f, 0.3f, -0.4f, 0.5f});
    float buf[5] = {};

    TEST_ASSERT_EQUAL_UINT(3, audio.readBlock(buf, 3));
    const float first[] = {0.1f, -0.2f, 0.3f};
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(first, buf, 3);

    buf[2] = 0.9f;
    TEST_ASSERT_EQUAL_UINT(2, audio.readBlock(buf, 5));
    const float last[] = {-0.4f, 0.5f};
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(last, buf, 2);
    TEST_ASSERT_EQUAL_FLOAT(0.9f, buf[2]);
    TEST_ASSERT_EQUAL_UINT(0, audio.readBlock(buf, 5));
}

void test_records_written_samples_in_order() {
    FakeAudioIo audio({});
    const float samples[] = {0.1f, -0.2f, 0.3f, -0.4f};

    TEST_ASSERT_EQUAL_UINT(4, audio.writeBlock(samples, 4));
    TEST_ASSERT_EQUAL_UINT(4, audio.written().size());
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(samples, audio.written().data(), 4);

    // Subsequent writes append rather than replacing the earlier block.
    const float tail[] = {0.5f};
    TEST_ASSERT_EQUAL_UINT(1, audio.writeBlock(tail, 1));
    TEST_ASSERT_EQUAL_UINT(5, audio.written().size());
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(samples, audio.written().data(), 4);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, audio.written()[4]);
}

void test_zero_length_operations_leave_audio_unchanged() {
    FakeAudioIo audio({0.25f});
    TEST_ASSERT_EQUAL_UINT(0, audio.readBlock(nullptr, 0));
    TEST_ASSERT_EQUAL_UINT(0, audio.writeBlock(nullptr, 0));
    TEST_ASSERT_TRUE(audio.written().empty());
    float sample = 0.0f;
    TEST_ASSERT_EQUAL_UINT(1, audio.readBlock(&sample, 1));
    TEST_ASSERT_EQUAL_FLOAT(0.25f, sample);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_reads_remaining_microphone_samples);
    RUN_TEST(test_records_written_samples_in_order);
    RUN_TEST(test_zero_length_operations_leave_audio_unchanged);
    return UNITY_END();
}
