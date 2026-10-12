#include "anc/calibration_recorder.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_records_until_full() {
    anc::CalibrationRecorder<8> rec;
    rec.start(5);
    TEST_ASSERT_FALSE(rec.full());

    for (int i = 0; i < 5; ++i) {
        TEST_ASSERT_TRUE(rec.add(0.1f * i, -0.2f * i));
    }
    TEST_ASSERT_FALSE(rec.add(9.0f, 9.0f));
    TEST_ASSERT_TRUE(rec.full());
    TEST_ASSERT_EQUAL_UINT(5, rec.size());

    for (int i = 0; i < 5; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(0.1f * i, rec.probe()[i]);
        TEST_ASSERT_EQUAL_FLOAT(-0.2f * i, rec.mic()[i]);
    }
}

void test_start_clamps_to_capacity() {
    anc::CalibrationRecorder<8> rec;
    rec.start(100);

    int accepted = 0;
    while (rec.add(1.0f, 1.0f)) {
        ++accepted;
    }
    TEST_ASSERT_EQUAL_INT(8, accepted);
    TEST_ASSERT_EQUAL_UINT(8, rec.size());
}

void test_start_clears_previous_recording() {
    anc::CalibrationRecorder<8> rec;
    rec.start(2);
    rec.add(1.0f, 1.0f);
    rec.add(1.0f, 1.0f);

    rec.start(3);
    TEST_ASSERT_EQUAL_UINT(0, rec.size());
    TEST_ASSERT_FALSE(rec.full());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_records_until_full);
    RUN_TEST(test_start_clamps_to_capacity);
    RUN_TEST(test_start_clears_previous_recording);
    return UNITY_END();
}
