// Rules every block processor must follow, checked only through the
// IBlockProcessor interface the filter task uses to swap between them.
#include <cstddef>

#include "anc/block_processor.hpp"
#include "anc/passthrough_processor.hpp"
#include "anc/silence_processor.hpp"
#include "anc/tone_processor.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr std::size_t kBlock = 32;
constexpr float kMarker      = 9.0f;  // outside [-1, 1], so an overwrite shows

void checkInRange(const float* out, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        TEST_ASSERT_TRUE_MESSAGE(out[i] >= -1.0f && out[i] <= 1.0f,
                                 "output outside [-1, 1]");
    }
}

// Alternating +1 and -1: the loudest mic block possible.
void fillFullScale(float* mic, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i)
        mic[i] = (i % 2 == 0) ? 1.0f : -1.0f;
}

void checkEmptyBlockLeavesOutputUntouched(anc::IBlockProcessor& p) {
    float mic[kBlock] = {};
    float out[kBlock];
    for (float& s : out)
        s = kMarker;

    p.processBlock(mic, out, 0);

    for (float s : out)
        TEST_ASSERT_EQUAL_FLOAT(kMarker, s);
}

void checkFullScaleBlockStaysInRange(anc::IBlockProcessor& p) {
    float mic[kBlock];
    float out[kBlock];
    fillFullScale(mic, kBlock);
    for (float& s : out)
        s = kMarker;

    p.processBlock(mic, out, kBlock);
    checkInRange(out, kBlock);
}

void checkTwoHalfBlocksStayInRange(anc::IBlockProcessor& p) {
    float mic[kBlock];
    float out[kBlock];
    fillFullScale(mic, kBlock);
    for (float& s : out)
        s = kMarker;

    p.processBlock(mic, out, kBlock / 2);
    p.processBlock(mic + kBlock / 2, out + kBlock / 2, kBlock / 2);
    checkInRange(out, kBlock);
}

void checkAllRules(anc::IBlockProcessor& p) {
    checkEmptyBlockLeavesOutputUntouched(p);
    checkFullScaleBlockStaysInRange(p);
    checkTwoHalfBlocksStayInRange(p);
}

void test_silence_processor() {
    anc::SilenceProcessor p;
    checkAllRules(p);
}

void test_passthrough_processor() {
    anc::PassthroughProcessor p(0.5f);
    checkAllRules(p);
}

void test_tone_processor() {
    anc::ToneProcessor p(8000.0f, 440.0f, 0.1f);
    checkAllRules(p);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_silence_processor);
    RUN_TEST(test_passthrough_processor);
    RUN_TEST(test_tone_processor);
    return UNITY_END();
}
