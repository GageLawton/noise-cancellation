#include "anc/reconstructor.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

// Static: the 1024-tap path filter is 8 KB.
anc::Reconstructor recon;

void test_removes_our_own_speaker() {
    const float path[] = {0.0f, 0.5f};
    recon.setSecondaryPath(path, 2);

    const float synth[] = {1.0f, 0.0f, 0.0f};
    const float mic[]   = {0.0f, 0.5f, 0.0f};  // exactly our own speaker
    for (int i = 0; i < 3; ++i) {
        TEST_ASSERT_EQUAL_FLOAT(0.0f, recon.process(mic[i], synth[i]));
    }
}

void test_silent_speaker_passes_mic_through() {
    const float path[] = {0.3f, 0.5f, -0.2f};
    recon.setSecondaryPath(path, 3);

    const float mic[] = {0.1f, -0.4f, 0.25f, 0.9f};
    for (float m : mic) {
        TEST_ASSERT_EQUAL_FLOAT(m, recon.process(m, 0.0f));
    }
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_removes_our_own_speaker);
    RUN_TEST(test_silent_speaker_passes_mic_through);
    return UNITY_END();
}
