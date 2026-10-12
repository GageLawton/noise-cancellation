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

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_impulse_returns_coefficients);
    return UNITY_END();
}
