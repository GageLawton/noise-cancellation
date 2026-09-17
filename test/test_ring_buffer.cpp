#include <thread>
#include <vector>

#include "anc/spsc_ring_buffer.hpp"
#include "unity.h"

using anc::SpscRingBuffer;

void setUp() {}
void tearDown() {}

namespace {

void test_empty_on_construction() {
    SpscRingBuffer<int, 8> rb;
    int out = 0;
    TEST_ASSERT_TRUE(rb.empty());
    TEST_ASSERT_FALSE(rb.pop(out));
}

void test_push_pop_preserves_order() {
    SpscRingBuffer<int, 8> rb;
    for (int i = 0; i < 5; ++i) {
        TEST_ASSERT_TRUE(rb.push(i));
    }
    for (int i = 0; i < 5; ++i) {
        int out = -1;
        TEST_ASSERT_TRUE(rb.pop(out));
        TEST_ASSERT_EQUAL_INT(i, out);
    }
    TEST_ASSERT_TRUE(rb.empty());
}

void test_push_fails_when_full_instead_of_blocking() {
    // This is the behavior FilterTask depends on: a full buffer must
    // return false so the caller can drop the sample, never block.
    using Rb = SpscRingBuffer<int, 4>;  // alias: the template comma
    Rb rb;                              // would split the macro args
                                        // usable capacity 3
    TEST_ASSERT_EQUAL_UINT(3, Rb::capacity());
    TEST_ASSERT_TRUE(rb.push(1));
    TEST_ASSERT_TRUE(rb.push(2));
    TEST_ASSERT_TRUE(rb.push(3));
    TEST_ASSERT_FALSE(rb.push(4));  // full: dropped, not blocked

    int out = 0;
    TEST_ASSERT_TRUE(rb.pop(out));  // freeing one slot
    TEST_ASSERT_EQUAL_INT(1, out);
    TEST_ASSERT_TRUE(rb.push(4));  // now fits
}

void test_wraps_around_repeatedly() {
    SpscRingBuffer<int, 4> rb;
    // Push/pop far more than capacity to exercise index wrapping.
    for (int i = 0; i < 100; ++i) {
        TEST_ASSERT_TRUE(rb.push(i));
        int out = -1;
        TEST_ASSERT_TRUE(rb.pop(out));
        TEST_ASSERT_EQUAL_INT(i, out);
    }
}

void test_size_tracks_contents() {
    SpscRingBuffer<int, 8> rb;
    TEST_ASSERT_EQUAL_UINT(0, rb.size());
    rb.push(1);
    rb.push(2);
    TEST_ASSERT_EQUAL_UINT(2, rb.size());
    int out = 0;
    rb.pop(out);
    TEST_ASSERT_EQUAL_UINT(1, rb.size());
}

void test_concurrent_producer_consumer() {
    // Approximates the real FilterTask -> AlignmentTask handoff: one
    // writer, one reader, no locks. Verifies no corruption or reordering.
    constexpr int kCount = 20000;
    SpscRingBuffer<int, 64> rb;

    std::vector<int> received;
    received.reserve(kCount);

    std::thread consumer([&] {
        int got = 0;
        while (static_cast<int>(received.size()) < kCount) {
            if (rb.pop(got)) {
                received.push_back(got);
            }
        }
    });

    for (int i = 0; i < kCount; ++i) {
        while (!rb.push(i)) {
            // Buffer full: spin. The real FilterTask drops instead --
            // here we retry so the test can assert nothing was lost.
        }
    }

    consumer.join();

    TEST_ASSERT_EQUAL_INT(kCount, static_cast<int>(received.size()));
    for (int i = 0; i < kCount; ++i) {
        TEST_ASSERT_EQUAL_INT(i, received[i]);
    }
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_empty_on_construction);
    RUN_TEST(test_push_pop_preserves_order);
    RUN_TEST(test_push_fails_when_full_instead_of_blocking);
    RUN_TEST(test_wraps_around_repeatedly);
    RUN_TEST(test_size_tracks_contents);
    RUN_TEST(test_concurrent_producer_consumer);
    return UNITY_END();
}
