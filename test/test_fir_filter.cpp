#include "anc/fir_filter.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_impulse_returns_coefficients() {
    anc::FirFilter<8> fir;
    const float coeffs[] = {0.5f, 0.25f, -0.125f};
    fir.setCoefficients(coeffs, 3);
    TEST_ASSERT_EQUAL_UINT(3, fir.taps());

    TEST_ASSERT_EQUAL_FLOAT(0.5f, fir.process(1.0f));
    TEST_ASSERT_EQUAL_FLOAT(0.25f, fir.process(0.0f));
    TEST_ASSERT_EQUAL_FLOAT(-0.125f, fir.process(0.0f));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, fir.process(0.0f));
}

void test_moving_average() {
    anc::FirFilter<8> fir;
    const float coeffs[] = {0.25f, 0.25f, 0.25f, 0.25f};
    fir.setCoefficients(coeffs, 4);

    const float expected[] = {1.0f, 2.0f, 3.0f, 4.0f, 4.0f};
    for (float want : expected) {
        TEST_ASSERT_EQUAL_FLOAT(want, fir.process(4.0f));
    }
}

void test_reset_clears_history() {
    anc::FirFilter<8> fir;
    const float coeffs[] = {0.5f, 0.25f, -0.125f};
    fir.setCoefficients(coeffs, 3);
    fir.process(1.0f);
    fir.process(-2.0f);

    fir.reset();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, fir.process(0.0f));
    TEST_ASSERT_EQUAL_UINT(3, fir.taps());
}

void test_full_length() {
    static anc::FirFilter<128> fir;
    float coeffs[128];
    for (float& c : coeffs) {
        c = 1.0f / 128.0f;
    }
    fir.setCoefficients(coeffs, 128);

    float out = 0.0f;
    for (int i = 0; i < 128; ++i) {
        out = fir.process(1.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, out);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_impulse_returns_coefficients);
    RUN_TEST(test_moving_average);
    RUN_TEST(test_reset_clears_history);
    RUN_TEST(test_full_length);
    return UNITY_END();
}
