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

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_unknown_node_returns_false);
    RUN_TEST(test_report_then_latest_returns_it);
    RUN_TEST(test_second_report_replaces_the_first);
    RUN_TEST(test_ninth_node_is_ignored);
    return UNITY_END();
}
