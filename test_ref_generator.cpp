#include "anc/ref_generator.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_first_sample_is_zero_sine_and_one_cosine() {
    anc::RefGenerator generator(8000.0f);
    generator.setFrequency(60.0f);

    float sinOut = -99.0f;
    float cosOut = -99.0f;

    generator.next(sinOut, cosOut);

    TEST_ASSERT_EQUAL_FLOAT(0.0f, sinOut);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, cosOut);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_first_sample_is_zero_sine_and_one_cosine);
    return UNITY_END();
}