#include "unity.h"
#include "output_limiter.h"

// Clipped needs to be false
namespace {

void test_output_limiter() {

    // Clamps x to [-ceiling, ceiling]. Sets clipped to true when it had to.
    inline float limitOutput(float x, float ceiling, bool& clipped);
    
    // 0.5f passes and clipped is false
    bool clipped = false;
    float result = limitOutput(0.5f, 1.0f, clipped);
    TEST_ASSERT_TRUE(result == 0.5f && clipped == false);
    // result should be 0.5f and clipped should be false
    
    // 1.2f gives 0.f and clipped is true
    clipped = true;
    result = limitOutput(1.2f, 1.0f, clipped);
    TEST_ASSERT_TRUE(result == 1.0f && clipped == true);
    
    // -3.0f gives 0.f and clipped is true
    clipped = true;
    result = limitOutput(-3.0f, 1.0f, clipped);
    TEST_ASSERT_TRUE(result == -1.0f && clipped == true);
    
    // 1.0f gives 0.8f and clipped is false
    clipped = false;
    result = limitOutput(1.0f, 0.8f, clipped);
    TEST_ASSERT_TRUE(result == 0.8f && clipped == false);
    // 1.0f gives 0.8f and clipped is false

}

} // namespace

int main() {
    UNITY_BEGIN();
    test_output_limiter();
    return UNITY_END();
}