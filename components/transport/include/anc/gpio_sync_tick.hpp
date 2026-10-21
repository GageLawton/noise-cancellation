#pragma once

#include <cstdint>

#include "anc/sync_tick.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace anc {

// The real sync line between boards (M7, design doc section 4): a short
// pulse on one GPIO marks a tick, and a rising edge on another GPIO is a
// tick from the other node. Use the pins reserved in pins.hpp: kPinSync1
// as the output and kPinSync2 as the input.
class GpioSyncTick final : public ISyncTick {
public:
    // Sets up both pins and the edge interrupt. Returns false if any
    // ESP-IDF call fails.
    bool begin(int outPin, int inPin);

    // Sends a pulse of about 10 µs on the output pin.
    void signal() override;

    // Waits up to timeoutMs for a rising edge on the input pin.
    bool wait(uint32_t timeoutMs) override;

private:
    static void onEdge(void* arg);

    int outPin_                = -1;
    int inPin_                 = -1;
    SemaphoreHandle_t arrived_ = nullptr;  // given by the ISR on each edge
};

}  // namespace anc
