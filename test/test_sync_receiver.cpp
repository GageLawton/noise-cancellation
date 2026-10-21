#include "anc/sync_receiver.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

anc::SyncMessage msg(uint32_t sequence) {
    return anc::SyncMessage{1, 1000ull * sequence, sequence};
}

void test_first_message_is_accepted_with_no_misses() {
    anc::SyncReceiver rx;
    TEST_ASSERT_TRUE(rx.accept(msg(7)));
    TEST_ASSERT_EQUAL_UINT32(1, rx.accepted());
    TEST_ASSERT_EQUAL_UINT32(0, rx.missed());
}

void test_in_order_stream_has_no_misses() {
    anc::SyncReceiver rx;
    for (uint32_t s = 1; s <= 5; ++s) {
        TEST_ASSERT_TRUE(rx.accept(msg(s)));
    }
    TEST_ASSERT_EQUAL_UINT32(5, rx.accepted());
    TEST_ASSERT_EQUAL_UINT32(0, rx.missed());
}

void test_gap_counts_missed_messages() {
    anc::SyncReceiver rx;
    TEST_ASSERT_TRUE(rx.accept(msg(1)));
    TEST_ASSERT_TRUE(rx.accept(msg(4)));
    TEST_ASSERT_EQUAL_UINT32(2, rx.missed());
    TEST_ASSERT_EQUAL_UINT32(2, rx.accepted());
}

void test_repeated_message_is_rejected() {
    anc::SyncReceiver rx;
    TEST_ASSERT_TRUE(rx.accept(msg(3)));
    TEST_ASSERT_FALSE(rx.accept(msg(3)));
    TEST_ASSERT_EQUAL_UINT32(1, rx.accepted());
    TEST_ASSERT_EQUAL_UINT32(0, rx.missed());
}

void test_older_message_after_newer_is_rejected() {
    anc::SyncReceiver rx;
    TEST_ASSERT_TRUE(rx.accept(msg(5)));
    TEST_ASSERT_FALSE(rx.accept(msg(4)));
    TEST_ASSERT_TRUE(rx.accept(msg(6)));
    TEST_ASSERT_EQUAL_UINT32(2, rx.accepted());
    TEST_ASSERT_EQUAL_UINT32(0, rx.missed());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_first_message_is_accepted_with_no_misses);
    RUN_TEST(test_in_order_stream_has_no_misses);
    RUN_TEST(test_gap_counts_missed_messages);
    RUN_TEST(test_repeated_message_is_rejected);
    RUN_TEST(test_older_message_after_newer_is_rejected);
    return UNITY_END();
}
