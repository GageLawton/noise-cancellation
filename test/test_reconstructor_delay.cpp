#include <cmath>

#include "anc/reconstructor.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

// Static: the 1024-tap path filter is 8 KB.
anc::Reconstructor recon;

constexpr int kSamples = 64;
constexpr int kDelay   = 3;
constexpr float kGain  = 0.5f;

float synthAt(int i) {
    return std::sin(0.3f * static_cast<float>(i));
}

// What the mic hears from our own speaker through a 3-sample, half-gain path.
float speakerAtMic(int i) {
    return i < kDelay ? 0.0f : kGain * synthAt(i - kDelay);
}

void setDelayedPath() {
    const float path[] = {0.0f, 0.0f, 0.0f, kGain};
    recon.setSecondaryPath(path, 4);
}

void test_removes_delayed_speaker() {
    setDelayedPath();
    for (int i = 0; i < kSamples; ++i)
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f,
                                 recon.process(speakerAtMic(i), synthAt(i)));
}

void test_keeps_room_noise_with_delayed_speaker() {
    setDelayedPath();
    const float roomNoise = 0.1f;
    for (int i = 0; i < kSamples; ++i)
        TEST_ASSERT_FLOAT_WITHIN(
            1e-6f, roomNoise, recon.process(speakerAtMic(i) + roomNoise, synthAt(i)));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_removes_delayed_speaker);
    RUN_TEST(test_keeps_room_noise_with_delayed_speaker);
    return UNITY_END();
}
