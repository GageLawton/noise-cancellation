#include "anc/output_limiter.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

// Every test uses the ceiling from issue #29.
constexpr float kCeiling = 0.8f;

void test_value_inside_ceiling_passes_through() {
    bool clipped = true;  // Start true so the test proves limitOutput clears it.
    float result = anc::limitOutput(0.5f, kCeiling, clipped);

    TEST_ASSERT_EQUAL_FLOAT(0.5f, result);
    TEST_ASSERT_FALSE(clipped);
}

void test_value_above_ceiling_is_clamped() {
    bool clipped = false;  // Start false so the test proves limitOutput sets it.
    float result = anc::limitOutput(1.2f, kCeiling, clipped);

    TEST_ASSERT_EQUAL_FLOAT(0.8f, result);
    TEST_ASSERT_TRUE(clipped);
}

void test_value_below_negative_ceiling_is_clamped() {
    bool clipped = false;
    float result = anc::limitOutput(-3.0f, kCeiling, clipped);

    TEST_ASSERT_EQUAL_FLOAT(-0.8f, result);
    TEST_ASSERT_TRUE(clipped);
}

void test_value_exactly_at_ceiling_is_not_clipped() {
    bool clipped = true;
    float result = anc::limitOutput(0.8f, kCeiling, clipped);

    TEST_ASSERT_EQUAL_FLOAT(0.8f, result);
    TEST_ASSERT_FALSE(clipped);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_value_inside_ceiling_passes_through);
    RUN_TEST(test_value_above_ceiling_is_clamped);
    RUN_TEST(test_value_below_negative_ceiling_is_clamped);
    RUN_TEST(test_value_exactly_at_ceiling_is_not_clipped);
    return UNITY_END();
}
