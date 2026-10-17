#include <cstdint>

#include "anc/crc8.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_check_value() {
    const uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    TEST_ASSERT_EQUAL_HEX8(0xF4, anc::crc8(data, sizeof(data)));
}

void test_empty_input_is_zero() {
    TEST_ASSERT_EQUAL_HEX8(0x00, anc::crc8(nullptr, 0));
}

void test_any_single_bit_flip_changes_the_crc() {
    uint8_t data[20];
    for (int i = 0; i < 20; ++i) {
        data[i] = static_cast<uint8_t>(i * 37 + 11);
    }
    const uint8_t original = anc::crc8(data, sizeof(data));

    for (int byte = 0; byte < 20; ++byte) {
        for (int bit = 0; bit < 8; ++bit) {
            data[byte] ^= static_cast<uint8_t>(1u << bit);
            TEST_ASSERT_NOT_EQUAL(original, anc::crc8(data, sizeof(data)));
            data[byte] ^= static_cast<uint8_t>(1u << bit);  // put it back
        }
    }
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_check_value);
    RUN_TEST(test_empty_input_is_zero);
    RUN_TEST(test_any_single_bit_flip_changes_the_crc);
    return UNITY_END();
}
