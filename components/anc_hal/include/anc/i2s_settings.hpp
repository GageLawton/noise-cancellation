#pragma once

#include "anc/node_config.hpp"
#include "anc/pins.hpp"

namespace anc {

struct I2sSettings {
    int sampleRateHz;
    int bclkPin;
    int wsPin;
    int doutPin;    // To the amp.
    int dinPin;     // From the mic.
    int dmaFrames;  // Samples per DMA buffer.
};

// Use the shared pin map and sample rate, with a 32-sample DMA block.
inline I2sSettings defaultI2sSettings() {
    return {static_cast<int>(kSampleRateHz),
            kPinI2sBclk,
            kPinI2sWs,
            kPinI2sDout,
            kPinI2sDin,
            32};
}

}  // namespace anc
