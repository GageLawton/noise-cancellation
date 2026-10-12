#include "fakes/simulated_room.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_speaker_reaches_mic_through_path() {
    const float path[] = {0.0f, 0.0f, 0.5f};
    SimulatedRoom room(8000.0f, path, 3);

    const float speaker[]  = {1.0f, 0.0f, 0.0f, 0.0f};
    const float expected[] = {0.0f, 0.0f, 0.5f, 0.0f};
    for (int i = 0; i < 4; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(expected[i], room.step(speaker[i]));
    }
}

void test_hum_reaches_mic() {
    const float path[] = {1.0f};
    SimulatedRoom room(8000.0f, path, 1);
    room.addHum(1000.0f, 1.0f, 0.0f);

    // 1000 Hz at 8000 Hz is 8 samples per cycle, so sample 2 is the peak.
    room.step(0.0f);
    room.step(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, room.step(0.0f));
}

void test_hums_add_up() {
    const float path[] = {1.0f};
    SimulatedRoom room(8000.0f, path, 1);
    for (int i = 0; i < 5; ++i) {  // the fifth is ignored
        room.addHum(1000.0f, 0.25f, 1.5707963f);
    }

    // At t = 0 each cosine-phased hum is at its peak of 0.25.
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, room.step(0.0f));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_speaker_reaches_mic_through_path);
    RUN_TEST(test_hum_reaches_mic);
    RUN_TEST(test_hums_add_up);
    return UNITY_END();
}
