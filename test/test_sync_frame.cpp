#include "anc/sync_frame.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

const anc::SyncMessage kSent{0xA1B2C3D4E5F60718ull, 123456789012345ull, 0xDEADBEEFu};

void test_round_trip() {
    uint8_t frame[anc::kSyncFrameBytes];
    anc::writeFrame(kSent, frame);

    anc::SyncMessage received{};
    TEST_ASSERT_TRUE(anc::readFrame(frame, sizeof(frame), received));
    TEST_ASSERT_TRUE(received.nodeId == kSent.nodeId);
    TEST_ASSERT_TRUE(received.timestampUs == kSent.timestampUs);
    TEST_ASSERT_EQUAL_HEX32(kSent.sequenceNumber, received.sequenceNumber);
}

void test_last_byte_is_crc_of_message() {
    uint8_t frame[anc::kSyncFrameBytes];
    anc::writeFrame(kSent, frame);
    TEST_ASSERT_EQUAL_HEX8(anc::crc8(frame, anc::kSyncMessageBytes),
                           frame[anc::kSyncMessageBytes]);
}

// CRC-8 catches every single-bit error, including one in the CRC byte.
void test_any_single_bit_flip_is_rejected() {
    uint8_t frame[anc::kSyncFrameBytes];
    anc::writeFrame(kSent, frame);

    for (std::size_t bit = 0; bit < anc::kSyncFrameBytes * 8; ++bit) {
        uint8_t corrupted[anc::kSyncFrameBytes];
        for (std::size_t i = 0; i < anc::kSyncFrameBytes; ++i)
            corrupted[i] = frame[i];
        corrupted[bit / 8] ^= static_cast<uint8_t>(1u << (bit % 8));

        anc::SyncMessage m{};
        TEST_ASSERT_FALSE_MESSAGE(anc::readFrame(corrupted, sizeof(corrupted), m),
                                  "a flipped bit was not caught");
    }
}

void test_wrong_length_is_rejected() {
    uint8_t frame[anc::kSyncFrameBytes + 1];
    anc::writeFrame(kSent, frame);
    anc::SyncMessage m{};
    TEST_ASSERT_FALSE(anc::readFrame(frame, anc::kSyncFrameBytes - 1, m));
    TEST_ASSERT_FALSE(anc::readFrame(frame, anc::kSyncFrameBytes + 1, m));
    TEST_ASSERT_FALSE(anc::readFrame(nullptr, anc::kSyncFrameBytes, m));
}

void test_rejected_frame_leaves_message_unchanged() {
    uint8_t frame[anc::kSyncFrameBytes];
    anc::writeFrame(kSent, frame);
    frame[0] ^= 0x01u;

    anc::SyncMessage m{1, 2, 3};
    TEST_ASSERT_FALSE(anc::readFrame(frame, sizeof(frame), m));
    TEST_ASSERT_TRUE(m.nodeId == 1);
    TEST_ASSERT_TRUE(m.timestampUs == 2);
    TEST_ASSERT_EQUAL_UINT32(3u, m.sequenceNumber);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_round_trip);
    RUN_TEST(test_last_byte_is_crc_of_message);
    RUN_TEST(test_any_single_bit_flip_is_rejected);
    RUN_TEST(test_wrong_length_is_rejected);
    RUN_TEST(test_rejected_frame_leaves_message_unchanged);
    return UNITY_END();
}
