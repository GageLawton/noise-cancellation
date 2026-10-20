#pragma once

#include "anc/node_config.hpp"
#include "anc/pins.hpp"

namespace anc {

struct I2sSettings {
    int sampleRateHz;
    int bclkPin;
    int wsPin;
    int doutPin;     // To the amp.
    int dinPin;      // From the mic.
    int dmaFrames;   // Samples per DMA buffer.
    int dmaBuffers;  // DMA buffers per direction (minimum 2).
};

// Use the shared pin map and sample rate, with a 32-sample DMA block.
//
// Only 3 DMA buffers (ESP-IDF's default is 6). The amp plays a block
// (dmaBuffers - 1) blocks after it is written, so every extra buffer adds
// 4 ms to the speaker-to-mic delay that calibration has to measure. Three
// still leaves one spare block in each direction if the audio task is late.
inline I2sSettings defaultI2sSettings() {
    return {static_cast<int>(kSampleRateHz),
            kPinI2sBclk,
            kPinI2sWs,
            kPinI2sDout,
            kPinI2sDin,
            32,
            3};
}

}  // namespace anc
