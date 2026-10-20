#pragma once

#include <cstddef>
#include <cstdint>

#include "anc/sync_frame.hpp"

// Host-only "wire" between two nodes for tests (M7). Frames go in at one
// end with send() and come out of receive() in the same order, so one
// test can play both sender and receiver.
class LoopbackLink {
public:
    static constexpr std::size_t kCapacity = 8;

    // Copies one frame in. False, and nothing stored, when the link is full.
    bool send(const uint8_t* frame) {
        if (count_ == kCapacity)
            return false;
        uint8_t* slot = frames_[(head_ + count_) % kCapacity];
        for (std::size_t i = 0; i < anc::kSyncFrameBytes; ++i) {
            slot[i] = frame[i];
        }
        if (corruptNext_) {
            slot[0] ^= 0x01;  // one flipped bit is enough for the CRC to catch
            corruptNext_ = false;
        }
        ++count_;
        return true;
    }

    // Copies the oldest frame out. False when the link is empty.
    bool receive(uint8_t* frame) {
        if (count_ == 0)
            return false;
        const uint8_t* slot = frames_[head_];
        for (std::size_t i = 0; i < anc::kSyncFrameBytes; ++i) {
            frame[i] = slot[i];
        }
        head_ = (head_ + 1) % kCapacity;
        --count_;
        return true;
    }

    // Test helper: flips one bit of the next frame sent.
    void corruptNext() { corruptNext_ = true; }

private:
    uint8_t frames_[kCapacity][anc::kSyncFrameBytes]{};
    std::size_t head_  = 0;  // oldest frame
    std::size_t count_ = 0;
    bool corruptNext_  = false;
};
