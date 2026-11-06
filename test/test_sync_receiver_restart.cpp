#include "anc/sync_receiver.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

anc::SyncMessage msg(uint32_t sequence) {
    return anc::SyncMessage{1, 1000ull * sequence, sequence};
}

// The sender rebooted after 3 and started counting from 1 again.
void test_restart_from_one_is_accepted() {
    anc::SyncReceiver rx;
    const uint32_t sequence[] = {1, 2, 3, 1, 2};
    for (uint32_t s : sequence) {
        TEST_ASSERT_TRUE(rx.accept(msg(s)));
    }
    TEST_ASSERT_EQUAL_UINT32(5, rx.accepted());
    TEST_ASSERT_EQUAL_UINT32(0, rx.missed());
}

void test_going_back_to_another_number_is_still_rejected() {
    anc::SyncReceiver rx;
    TEST_ASSERT_TRUE(rx.accept(msg(1)));
    TEST_ASSERT_TRUE(rx.accept(msg(2)));
    TEST_ASSERT_TRUE(rx.accept(msg(3)));
    TEST_ASSERT_FALSE(rx.accept(msg(2)));
    TEST_ASSERT_EQUAL_UINT32(3, rx.accepted());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_restart_from_one_is_accepted);
    RUN_TEST(test_going_back_to_another_number_is_still_rejected);
    return UNITY_END();
}
