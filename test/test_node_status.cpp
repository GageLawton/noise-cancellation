#include "anc/node_status.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_read_before_publish_is_zeroed() {
    anc::NodeStatusStore store;
    anc::NodeStatus result = store.read();

    TEST_ASSERT_EQUAL_UINT64(0, result.nodeId);
    TEST_ASSERT_EQUAL_UINT32(0, result.clipEvents);
}

void test_read_returns_published_values() {
    anc::NodeStatusStore store;
    anc::NodeStatus reported{};
    reported.nodeId     = 123456789ULL;
    reported.state      = anc::NodeState::Run;
    reported.clipEvents = 7;
    reported.numTones   = 2;

    store.publish(reported);
    anc::NodeStatus result = store.read();

    TEST_ASSERT_EQUAL_UINT64(reported.nodeId, result.nodeId);
    TEST_ASSERT_TRUE(result.state == reported.state);
    TEST_ASSERT_EQUAL_UINT32(reported.clipEvents, result.clipEvents);
    TEST_ASSERT_EQUAL_UINT8(reported.numTones, result.numTones);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_read_before_publish_is_zeroed);
    RUN_TEST(test_read_returns_published_values);
    return UNITY_END();
}
