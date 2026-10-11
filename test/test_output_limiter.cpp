#include "anc/output_limiter.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_passes_through_below_ceiling() {
    bool clipped = true;
    float output = anc::limitOutput(0.5f, 0.8f, clipped);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, output);
    TEST_ASSERT_FALSE(clipped);
}

void test_clamps_above_ceiling() {
    bool clipped = false;
    float output = anc::limitOutput(1.2f, 0.8f, clipped);
    TEST_ASSERT_EQUAL_FLOAT(0.8f, output);
    TEST_ASSERT_TRUE(clipped);
}

void test_clamps_below_negative_ceiling() {
    bool clipped = false;
    float output = anc::limitOutput(-3.0f, 0.8f, clipped);
    TEST_ASSERT_EQUAL_FLOAT(-0.8f, output);
    TEST_ASSERT_TRUE(clipped);
}

void test_passes_through_exact_ceiling() {
    bool clipped = true;
    float output = anc::limitOutput(0.8f, 0.8f, clipped);
    TEST_ASSERT_EQUAL_FLOAT(0.8f, output);
    TEST_ASSERT_FALSE(clipped);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_passes_through_below_ceiling);
    RUN_TEST(test_clamps_above_ceiling);
    RUN_TEST(test_clamps_below_negative_ceiling);
    RUN_TEST(test_passes_through_exact_ceiling);
    return UNITY_END();
}
