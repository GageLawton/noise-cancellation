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

void test_forget_drops_only_that_node() {
    anc::LocalCoordinator coordinator;
    coordinator.report(statusFor(1, 10));
    coordinator.report(statusFor(2, 20));
    coordinator.report(statusFor(3, 30));

    TEST_ASSERT_TRUE(coordinator.forget(2));
    TEST_ASSERT_EQUAL_size_t(2, coordinator.nodeCount());

    anc::NodeStatus out{};
    TEST_ASSERT_FALSE(coordinator.latest(2, out));
    TEST_ASSERT_TRUE(coordinator.latest(1, out));
    TEST_ASSERT_EQUAL_UINT32(10, out.clipEvents);
    TEST_ASSERT_TRUE(coordinator.latest(3, out));
    TEST_ASSERT_EQUAL_UINT32(30, out.clipEvents);
}

void test_forget_unknown_node_changes_nothing() {
    anc::LocalCoordinator coordinator;
    coordinator.report(statusFor(1, 10));

    TEST_ASSERT_FALSE(coordinator.forget(99));
    TEST_ASSERT_EQUAL_size_t(1, coordinator.nodeCount());
    anc::NodeStatus out{};
    TEST_ASSERT_TRUE(coordinator.latest(1, out));
}

void test_forget_frees_a_slot_when_full() {
    anc::LocalCoordinator coordinator;
    for (uint64_t id = 1; id <= anc::LocalCoordinator::kMaxNodes; ++id) {
        coordinator.report(statusFor(id, 0));
    }
    TEST_ASSERT_TRUE(coordinator.forget(4));

    coordinator.report(statusFor(100, 5));
    anc::NodeStatus out{};
    TEST_ASSERT_TRUE(coordinator.latest(100, out));
    TEST_ASSERT_EQUAL_UINT32(5, out.clipEvents);
    TEST_ASSERT_EQUAL_size_t(anc::LocalCoordinator::kMaxNodes, coordinator.nodeCount());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_forget_drops_only_that_node);
    RUN_TEST(test_forget_unknown_node_changes_nothing);
    RUN_TEST(test_forget_frees_a_slot_when_full);
    return UNITY_END();
}
