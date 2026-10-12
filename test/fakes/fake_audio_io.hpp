#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include "anc/audio_io.hpp"

// Host-only microphone playback and speaker capture for audio tests.
class FakeAudioIo final : public anc::IAudioIo {
public:
    explicit FakeAudioIo(std::vector<float> micSamples)
        : micSamples_(std::move(micSamples)) {}

    std::size_t readBlock(float* buf, std::size_t n) override {
        const std::size_t count = std::min(n, micSamples_.size() - readPosition_);
        for (std::size_t i = 0; i < count; ++i) {
            buf[i] = micSamples_[readPosition_ + i];
        }
        readPosition_ += count;
        return count;
    }

    std::size_t writeBlock(const float* buf, std::size_t n) override {
        if (n > 0) {
            written_.insert(written_.end(), buf, buf + n);
        }
        return n;
    }

    const std::vector<float>& written() const { return written_; }

private:
    std::vector<float> micSamples_;
    std::size_t readPosition_ = 0;
    std::vector<float> written_;
};
