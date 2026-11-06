#include "anc/sync_receiver.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

anc::SyncMessage msg(uint32_t sequence) {
    return anc::SyncMessage{1, 1000ull * sequence, sequence};
}

void test_steady_stream_has_no_restarts() {
    anc::SyncReceiver rx;
    for (uint32_t s = 1; s <= 3; ++s) {
        rx.accept(msg(s));
    }
    TEST_ASSERT_EQUAL_UINT32(0, rx.restarts());
}

void test_each_return_to_one_counts_a_restart() {
    anc::SyncReceiver rx;
    const uint32_t sequence[] = {1, 2, 1, 2, 1};
    for (uint32_t s : sequence) {
        rx.accept(msg(s));
    }
    TEST_ASSERT_EQUAL_UINT32(2, rx.restarts());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_steady_stream_has_no_restarts);
    RUN_TEST(test_each_return_to_one_counts_a_restart);
    return UNITY_END();
}
