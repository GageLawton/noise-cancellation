#pragma once

#include <cstddef>
#include <cstdint>

#include "anc/audio_io.hpp"
#include "anc/i2s_settings.hpp"
#include "driver/i2s_std.h"

namespace anc {

// The real ESP32 audio driver: one full-duplex I2S port, with the amp on
// the TX side and the mic on the RX side, sharing BCLK and WS.
class I2sAudioIo final : public IAudioIo {
public:
    // Returns false on any ESP-IDF error, so the caller decides what to do.
    bool init(const I2sSettings& settings);
    std::size_t readBlock(float* buf, std::size_t n) override;
    std::size_t writeBlock(const float* buf, std::size_t n) override;

private:
    static constexpr std::size_t kMaxBlock = 256;  // dma_block_samples' upper limit

    // Frees both channels after a failed init.
    void release();

    i2s_chan_handle_t tx_ = nullptr;
    i2s_chan_handle_t rx_ = nullptr;
    int32_t rawIn_[kMaxBlock]{};
    int32_t rawOut_[kMaxBlock]{};
    bool readErrorLogged_ = false;
};

}  // namespace anc
