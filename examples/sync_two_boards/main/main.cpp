// Sync tick between two boards (issue #302).
//
// Flash this same app to both boards. Wire board 1 GPIO15 (kPinSync1, out)
// to board 2 GPIO16 (kPinSync2, in), and GND to GND. Every board sends a
// burst of 1000 ticks, one every 100 ms, and listens for ticks at the same
// time. The board on the receiving end of the wire prints how many arrived
// and the largest gap between two of them; with no lost ticks that is close
// to 100 ms. Running the same app on both means no role to set, and a
// second wire (board 2 GPIO15 to board 1 GPIO16) checks the other direction.

#include <cinttypes>
#include <cstdio>

#include "anc/gpio_sync_tick.hpp"
#include "anc/pins.hpp"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr int kTicksPerBurst     = 1000;
constexpr uint32_t kTickPeriodMs = 100;
// Pause between bursts, so the receiver can tell where one burst ends.
constexpr uint32_t kBurstGapMs = 5000;
// A gap this long with nothing heard ends the receiver's count.
constexpr int64_t kQuietUs = 2000000;
// A gap longer than one and a half periods means a tick went missing.
constexpr int64_t kMissedGapUs = kTickPeriodMs * 1000 * 3 / 2;

anc::GpioSyncTick tick;

void sendTask(void*) {
    for (int burst = 1;; ++burst) {
        TickType_t wake = xTaskGetTickCount();
        for (int i = 0; i < kTicksPerBurst; ++i) {
            tick.signal();
            vTaskDelayUntil(&wake, pdMS_TO_TICKS(kTickPeriodMs));
        }
        std::printf("sent burst %d: %d ticks\n", burst, kTicksPerBurst);
        vTaskDelay(pdMS_TO_TICKS(kBurstGapMs));
    }
}

}  // namespace

extern "C" void app_main() {
    if (!tick.begin(anc::kPinSync1, anc::kPinSync2)) {
        std::printf("sync_two_boards: GpioSyncTick::begin failed\n");
        return;
    }
    std::printf(
        "sync_two_boards: sending on GPIO%d, listening on GPIO%d, "
        "%d ticks every %" PRIu32 " ms\n",
        anc::kPinSync1, anc::kPinSync2, kTicksPerBurst, kTickPeriodMs);
    xTaskCreate(sendTask, "sync_send", 3072, nullptr, 5, nullptr);

    int received      = 0;
    int missedGaps    = 0;
    int64_t largestUs = 0;
    int64_t lastUs    = 0;
    int64_t idleUs    = esp_timer_get_time();
    for (;;) {
        const bool heard  = tick.wait(200);
        const int64_t now = esp_timer_get_time();
        if (heard) {
            if (received > 0) {
                const int64_t gapUs = now - lastUs;
                if (gapUs > largestUs) {
                    largestUs = gapUs;
                }
                if (gapUs > kMissedGapUs) {
                    ++missedGaps;
                }
            }
            ++received;
            lastUs = now;
        } else if (received > 0 && now - lastUs > kQuietUs) {
            std::printf("received %d/%d, largest gap %" PRId64 " us, gaps over %" PRId64
                        " us: %d\n",
                        received, kTicksPerBurst, largestUs, kMissedGapUs, missedGaps);
            received   = 0;
            missedGaps = 0;
            largestUs  = 0;
            idleUs     = now;
        } else if (received == 0 && now - idleUs > 10000000) {
            std::printf(
                "no ticks heard on GPIO%d (is this the receiving board, "
                "and is GND shared?)\n",
                anc::kPinSync2);
            idleUs = now;
        }
    }
}
