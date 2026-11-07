// Sync tick loopback check (issue #267).
//
// Jumper GPIO15 (kPinSync1, out) to GPIO16 (kPinSync2, in). The board sends
// 1000 ticks to itself and counts how many arrive, then prints the result
// and the worst round trip. With the jumper removed it should hear none.

#include <cinttypes>
#include <cstdio>

#include "anc/gpio_sync_tick.hpp"
#include "anc/pins.hpp"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr int kRounds         = 1000;
constexpr uint32_t kTimeoutMs = 10;

}  // namespace

extern "C" void app_main() {
    anc::GpioSyncTick tick;
    if (!tick.begin(anc::kPinSync1, anc::kPinSync2)) {
        std::printf("sync_loopback: GpioSyncTick::begin failed\n");
        return;
    }
    std::printf("sync_loopback: GPIO%d -> GPIO%d, %d rounds\n", anc::kPinSync1,
                anc::kPinSync2, kRounds);

    for (int run = 1;; ++run) {
        int received    = 0;
        int64_t worstUs = 0;
        int64_t totalUs = 0;
        for (int i = 0; i < kRounds; ++i) {
            const int64_t start = esp_timer_get_time();
            tick.signal();
            if (tick.wait(kTimeoutMs)) {
                const int64_t tripUs = esp_timer_get_time() - start;
                ++received;
                totalUs += tripUs;
                if (tripUs > worstUs) {
                    worstUs = tripUs;
                }
            }
            // Leave a gap so one pulse can't count for two rounds.
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        if (received > 0) {
            std::printf("run %d: received %d/%d, worst round trip %" PRId64
                        " us, mean %" PRId64 " us\n",
                        run, received, kRounds, worstUs, totalUs / received);
        } else {
            std::printf("run %d: received 0/%d (no jumper?)\n", run, kRounds);
        }
        // Run again every few seconds, so you can pull the jumper and watch
        // the count drop to 0 without reflashing.
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
