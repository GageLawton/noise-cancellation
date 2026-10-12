#include <algorithm>
#include <cmath>
#include <cstdio>

#include "anc/decimator2x.hpp"
#include "anc/upsampler2x.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr float kTwoPi     = 6.28318530718f;
constexpr float kRate      = 8000.0f;
constexpr std::size_t kLen = 800;  // 100 ms at 8 kHz

float in[kLen];
float up[2 * kLen];
float out[kLen];

// Makes a 100 Hz sine at 8 kHz, doubles its rate, then halves it again.
// Returns how many samples came back.
std::size_t roundTrip() {
    for (std::size_t i = 0; i < kLen; ++i) {
        in[i] = std::sin(kTwoPi * 100.0f * static_cast<float>(i) / kRate);
    }
    anc::Upsampler2x upsampler;
    anc::Decimator2x decimator;
    const std::size_t upCount = upsampler.process(in, kLen, up);
    return decimator.process(up, upCount, out);
}

void test_round_trip_keeps_a_100hz_tone() {
    TEST_ASSERT_EQUAL_size_t(kLen, roundTrip());

    // Skip the first 40 samples while the two filters fill up.
    float peak = 0.0f;
    for (std::size_t i = 40; i < kLen; ++i) {
        peak = std::max(peak, std::fabs(out[i]));
    }
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, peak);
}

void test_round_trip_delay_is_constant() {
    roundTrip();

    // Try every shift d and keep the one where out[n] best matches
    // in[n - d] over samples 100 to 700.
    constexpr std::size_t kFirst = 100;
    constexpr std::size_t kLast  = 700;
    std::size_t bestDelay        = 0;
    double bestError             = 1e30;
    for (std::size_t d = 0; d <= 20; ++d) {
        double error = 0.0;
        for (std::size_t n = kFirst; n < kLast; ++n) {
            const double diff = out[n] - in[n - d];
            error += diff * diff;
        }
        if (error < bestError) {
            bestError = error;
            bestDelay = d;
        }
    }

    printf("round-trip delay: %zu samples, mean squared error %g\n", bestDelay,
           bestError / (kLast - kFirst));
    // A fixed delay is fine (calibration measures it); the shape must match.
    TEST_ASSERT_TRUE(bestError / (kLast - kFirst) < 0.01);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_round_trip_keeps_a_100hz_tone);
    RUN_TEST(test_round_trip_delay_is_constant);
    return UNITY_END();
}
