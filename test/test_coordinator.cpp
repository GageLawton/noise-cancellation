#include "anc/coordinator.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

anc::NodeStatus statusFor(uint64_t nodeId, uint32_t clipEvents) {
    anc::NodeStatus s{};
    s.nodeId     = nodeId;
    s.clipEvents = clipEvents;
    return s;
}

void test_unknown_node_returns_false() {
    anc::LocalCoordinator coordinator;
    anc::NodeStatus out{};
    TEST_ASSERT_FALSE(coordinator.latest(42, out));
    TEST_ASSERT_EQUAL_size_t(0, coordinator.nodeCount());
}

void test_report_then_latest_returns_it() {
    anc::LocalCoordinator coordinator;
    coordinator.report(statusFor(42, 7));

    anc::NodeStatus out{};
    TEST_ASSERT_TRUE(coordinator.latest(42, out));
    TEST_ASSERT_EQUAL_UINT64(42, out.nodeId);
    TEST_ASSERT_EQUAL_UINT32(7, out.clipEvents);
}

void test_second_report_replaces_the_first() {
    anc::LocalCoordinator coordinator;
    coordinator.report(statusFor(42, 7));
    coordinator.report(statusFor(42, 9));

    TEST_ASSERT_EQUAL_size_t(1, coordinator.nodeCount());
    anc::NodeStatus out{};
    TEST_ASSERT_TRUE(coordinator.latest(42, out));
    TEST_ASSERT_EQUAL_UINT32(9, out.clipEvents);
}

void test_ninth_node_is_ignored() {
    anc::LocalCoordinator coordinator;
    anc::ICoordinator& asInterface = coordinator;  // callers only see the interface
    for (uint64_t id = 1; id <= 9; ++id) {
        asInterface.report(statusFor(id, 0));
    }

    TEST_ASSERT_EQUAL_size_t(8, asInterface.nodeCount());
    anc::NodeStatus out{};
    TEST_ASSERT_TRUE(asInterface.latest(8, out));
    TEST_ASSERT_FALSE(asInterface.latest(9, out));
}

anc::NodeStatus statusAt(uint64_t nodeId, uint32_t timestampMs) {
    anc::NodeStatus s{};
    s.nodeId      = nodeId;
    s.timestampMs = timestampMs;
    return s;
}

void test_stale_count_finds_the_quiet_node() {
    anc::LocalCoordinator coordinator;
    coordinator.report(statusAt(1, 1000));
    coordinator.report(statusAt(2, 5000));

    // Node 1 is 5000 ms old, node 2 only 1000 ms.
    TEST_ASSERT_EQUAL_size_t(1, coordinator.staleCount(6000, 2000));
}

void test_stale_count_survives_counter_wraparound() {
    anc::LocalCoordinator coordinator;
    coordinator.report(statusAt(1, 0xFFFFFF00u));

    // The counter wrapped: the report is only 0x200 = 512 ms old.
    TEST_ASSERT_EQUAL_size_t(0, coordinator.staleCount(0x00000100u, 1000));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_unknown_node_returns_false);
    RUN_TEST(test_report_then_latest_returns_it);
    RUN_TEST(test_second_report_replaces_the_first);
    RUN_TEST(test_ninth_node_is_ignored);
    RUN_TEST(test_stale_count_finds_the_quiet_node);
    RUN_TEST(test_stale_count_survives_counter_wraparound);
    return UNITY_END();
}
