#include <cstddef>

#include "anc/pins.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr int kPins[]           = {anc::kPinI2sBclk, anc::kPinI2sWs, anc::kPinI2sDout,
                                   anc::kPinI2sDin,  anc::kPinSync1, anc::kPinSync2};
constexpr std::size_t kPinCount = sizeof(kPins) / sizeof(kPins[0]);

bool isReserved(int pin) {
    return pin == 0 || pin == 3 || pin == 45 || pin == 46 || (pin >= 26 && pin <= 37) ||
           pin == 19 || pin == 20 || pin == 43 || pin == 44;
}

void test_pins_are_distinct() {
    for (std::size_t i = 0; i < kPinCount; ++i) {
        for (std::size_t j = i + 1; j < kPinCount; ++j) {
            TEST_ASSERT_NOT_EQUAL(kPins[i], kPins[j]);
        }
    }
}

void test_pins_avoid_reserved_gpios() {
    for (int pin : kPins) {
        TEST_ASSERT_FALSE(isReserved(pin));
    }
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_pins_are_distinct);
    RUN_TEST(test_pins_avoid_reserved_gpios);
    return UNITY_END();
}
