#include <cstddef>
#include <cstdint>
#include <cstring>

#include "anc/sync_frame.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr std::size_t kFrameBits = 8 * anc::kSyncFrameBytes;  // 168

const anc::SyncMessage kSent{0xA1B2C3D4E5F60718ull, 123456789012345ull, 0xDEADBEEFu};

// Bits are numbered most significant first within each byte, the order
// the CRC reads them, so a burst stays contiguous across byte boundaries.
void flipBit(uint8_t* frame, std::size_t bit) {
    frame[bit / 8] ^= static_cast<uint8_t>(0x80u >> (bit % 8));
}

bool accepted(const uint8_t* frame) {
    anc::SyncMessage received{};
    return anc::readFrame(frame, anc::kSyncFrameBytes, received);
}

void test_intact_frame_is_accepted() {
    uint8_t frame[anc::kSyncFrameBytes];
    anc::writeFrame(kSent, frame);
    TEST_ASSERT_TRUE(accepted(frame));
}

// An 8-bit CRC catches every burst of up to 8 bits. The first and last
// bit of the burst are flipped; the ones between take every pattern.
void test_every_burst_up_to_8_bits_is_rejected() {
    uint8_t sent[anc::kSyncFrameBytes];
    anc::writeFrame(kSent, sent);

    std::size_t cases = 0;
    for (std::size_t length = 2; length <= 8; ++length) {
        const unsigned patterns = 1u << (length - 2);
        for (std::size_t start = 0; start + length <= kFrameBits; ++start) {
            for (unsigned inner = 0; inner < patterns; ++inner) {
                uint8_t frame[anc::kSyncFrameBytes];
                std::memcpy(frame, sent, sizeof(frame));
                flipBit(frame, start);
                flipBit(frame, start + length - 1);
                for (std::size_t i = 0; i + 2 < length; ++i) {
                    if (inner & (1u << i)) {
                        flipBit(frame, start + 1 + i);
                    }
                }
                TEST_ASSERT_FALSE(accepted(frame));
                ++cases;
            }
        }
    }
    // Sum over L = 2..8 of (169 - L) * 2^(L-2).
    TEST_ASSERT_EQUAL_UINT(20567, cases);
}

// Polynomial 0x07 has (x + 1) as a factor, so it catches any odd number
// of flipped bits; check every way of flipping exactly 3.
void test_every_three_bit_flip_is_rejected() {
    uint8_t frame[anc::kSyncFrameBytes];
    anc::writeFrame(kSent, frame);

    std::size_t cases = 0;
    for (std::size_t a = 0; a < kFrameBits; ++a) {
        for (std::size_t b = a + 1; b < kFrameBits; ++b) {
            for (std::size_t c = b + 1; c < kFrameBits; ++c) {
                flipBit(frame, a);
                flipBit(frame, b);
                flipBit(frame, c);
                TEST_ASSERT_FALSE(accepted(frame));
                flipBit(frame, a);  // put them back
                flipBit(frame, b);
                flipBit(frame, c);
                ++cases;
            }
        }
    }
    TEST_ASSERT_EQUAL_UINT(776216, cases);  // 168 choose 3
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_intact_frame_is_accepted);
    RUN_TEST(test_every_burst_up_to_8_bits_is_rejected);
    RUN_TEST(test_every_three_bit_flip_is_rejected);
    return UNITY_END();
}
