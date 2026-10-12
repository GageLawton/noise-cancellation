#include "anc/sync_message.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_round_trip() {
    const anc::SyncMessage sent{0xA1B2C3D4E5F60718ull, 123456789012345ull, 0xDEADBEEFu};
    uint8_t bytes[anc::kSyncMessageBytes];
    anc::serialize(sent, bytes);

    anc::SyncMessage received{};
    TEST_ASSERT_TRUE(anc::deserialize(bytes, sizeof(bytes), received));
    TEST_ASSERT_TRUE(received.nodeId == sent.nodeId);
    TEST_ASSERT_TRUE(received.timestampUs == sent.timestampUs);
    TEST_ASSERT_EQUAL_HEX32(sent.sequenceNumber, received.sequenceNumber);
}

void test_little_endian_byte_order() {
    const anc::SyncMessage m{0x0102030405060708ull, 0, 0x0A0B0C0Du};
    uint8_t bytes[anc::kSyncMessageBytes];
    anc::serialize(m, bytes);
    TEST_ASSERT_EQUAL_HEX8(0x08, bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, bytes[7]);
    TEST_ASSERT_EQUAL_HEX8(0x0D, bytes[16]);
    TEST_ASSERT_EQUAL_HEX8(0x0A, bytes[19]);
}

void test_wrong_length_is_rejected() {
    const uint8_t bytes[anc::kSyncMessageBytes] = {};
    anc::SyncMessage m{1, 2, 3};
    TEST_ASSERT_FALSE(anc::deserialize(bytes, 19, m));
    TEST_ASSERT_FALSE(anc::deserialize(bytes, 21, m));
    TEST_ASSERT_FALSE(anc::deserialize(nullptr, anc::kSyncMessageBytes, m));
    TEST_ASSERT_TRUE(m.nodeId == 1);  // untouched
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_round_trip);
    RUN_TEST(test_little_endian_byte_order);
    RUN_TEST(test_wrong_length_is_rejected);
    return UNITY_END();
}
