#include "anc/sync_frame.hpp"
#include "fakes/loopback_link.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

anc::SyncMessage msg(uint32_t sequence) {
    return anc::SyncMessage{0x1122334455667788ull, 1000ull * sequence, sequence};
}

bool sendMessage(LoopbackLink& link, const anc::SyncMessage& m) {
    uint8_t frame[anc::kSyncFrameBytes];
    anc::writeFrame(m, frame);
    return link.send(frame);
}

void test_frames_come_back_equal_and_in_order() {
    LoopbackLink link;
    for (uint32_t s = 1; s <= 3; ++s) {
        TEST_ASSERT_TRUE(sendMessage(link, msg(s)));
    }

    for (uint32_t s = 1; s <= 3; ++s) {
        uint8_t frame[anc::kSyncFrameBytes];
        TEST_ASSERT_TRUE(link.receive(frame));
        anc::SyncMessage got{};
        TEST_ASSERT_TRUE(anc::readFrame(frame, sizeof(frame), got));
        const anc::SyncMessage want = msg(s);
        TEST_ASSERT_TRUE(got.nodeId == want.nodeId);
        TEST_ASSERT_TRUE(got.timestampUs == want.timestampUs);
        TEST_ASSERT_EQUAL_UINT32(want.sequenceNumber, got.sequenceNumber);
    }

    uint8_t frame[anc::kSyncFrameBytes];
    TEST_ASSERT_FALSE(link.receive(frame));  // empty now
}

void test_corrupted_frame_is_rejected_and_next_is_fine() {
    LoopbackLink link;
    link.corruptNext();
    TEST_ASSERT_TRUE(sendMessage(link, msg(1)));
    TEST_ASSERT_TRUE(sendMessage(link, msg(2)));

    uint8_t frame[anc::kSyncFrameBytes];
    anc::SyncMessage got{};
    TEST_ASSERT_TRUE(link.receive(frame));
    TEST_ASSERT_FALSE(anc::readFrame(frame, sizeof(frame), got));

    TEST_ASSERT_TRUE(link.receive(frame));
    TEST_ASSERT_TRUE(anc::readFrame(frame, sizeof(frame), got));
    TEST_ASSERT_EQUAL_UINT32(2, got.sequenceNumber);
}

void test_ninth_frame_is_refused_when_full() {
    LoopbackLink link;
    for (uint32_t s = 1; s <= 8; ++s) {
        TEST_ASSERT_TRUE(sendMessage(link, msg(s)));
    }
    TEST_ASSERT_FALSE(sendMessage(link, msg(9)));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_frames_come_back_equal_and_in_order);
    RUN_TEST(test_corrupted_frame_is_rejected_and_next_is_fine);
    RUN_TEST(test_ninth_frame_is_refused_when_full);
    return UNITY_END();
}
