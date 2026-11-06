#include "anc/reconstructor.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

// Static: the 1024-tap path filter is 8 KB.
anc::Reconstructor recon;

void test_reset_clears_old_speaker_samples() {
    const float path[] = {0.0f, 1.0f};  // one-sample delay
    recon.setSecondaryPath(path, 2);
    recon.process(0.0f, 1.0f);
    recon.reset();
    // Without the reset, the 1.0 played last sample would come back as -1.
    TEST_ASSERT_EQUAL_FLOAT(0.0f, recon.process(0.0f, 0.0f));
}

void test_reset_keeps_the_path() {
    const float path[] = {0.0f, 1.0f};
    recon.setSecondaryPath(path, 2);
    recon.reset();
    recon.process(1.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, recon.process(1.0f, 0.0f));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_reset_clears_old_speaker_samples);
    RUN_TEST(test_reset_keeps_the_path);
    return UNITY_END();
}
