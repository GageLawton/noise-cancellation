#include "fakes/fake_sync_tick.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_wait_is_false_with_nothing_delivered() {
    FakeSyncTick tick;
    TEST_ASSERT_FALSE(tick.wait(100));
}

void test_delivered_tick_is_seen_once() {
    FakeSyncTick tick;
    tick.deliverTick();
    TEST_ASSERT_TRUE(tick.wait(100));
    TEST_ASSERT_FALSE(tick.wait(100));
}

void test_signal_is_counted() {
    FakeSyncTick tick;
    anc::ISyncTick& asInterface = tick;  // callers only see the interface
    asInterface.signal();
    asInterface.signal();
    TEST_ASSERT_EQUAL_UINT32(2, tick.signals());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_wait_is_false_with_nothing_delivered);
    RUN_TEST(test_delivered_tick_is_seen_once);
    RUN_TEST(test_signal_is_counted);
    return UNITY_END();
}
