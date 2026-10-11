#include <stdio.h>
#include "driver/i2s_std.h"
#include "esp_err.h"

// Breadboard wiring (see hardware/breadboard-wiring guide).
// The mic and the amp share BCLK and WS, so both sides run off one clock.
#define PIN_BCLK GPIO_NUM_4   // bit clock: mic SCK + amp BCLK
#define PIN_WS   GPIO_NUM_5   // word select (left/right): mic WS + amp LRC
#define PIN_DOUT GPIO_NUM_6   // ESP32 -> amp DIN (speaker data)
#define PIN_DIN  GPIO_NUM_7   // mic SD -> ESP32 (microphone data)

#define SAMPLE_RATE_HZ 16000

extern "C" {

    void app_main() {

        // Part 1: create the channel pair.
        // One MASTER config on port 0 gives two handles (tx for the amp, rx for the mic)
        // that share the same BCLK/WS clock. ESP-IDF reports errors by returning an
        // esp_err_t; ESP_ERROR_CHECK prints the error and reboots if it isn't ESP_OK.
        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        i2s_chan_handle_t tx, rx;
        ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx, &rx));

        // Part 2: describe how the bus talks. Three pieces, in the order the struct declares them.
        i2s_std_config_t std_cfg = {
            // Sample rate. With two 32-bit slots per frame, BCLK = 16000 * 64 = 1.024 MHz.
            .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE_HZ),
            // Standard (Philips) I2S framing, 32-bit slots, one channel.
            // The INMP441 sends 24-bit audio left-aligned inside a 32-bit slot.
            .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
            .gpio_cfg = {
                .mclk = I2S_GPIO_UNUSED,  // the mic and amp make their own clocks from BCLK
                .bclk = PIN_BCLK,
                .ws   = PIN_WS,
                .dout = PIN_DOUT,
                .din  = PIN_DIN,
                .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
            },
        };
        // The mic's L/R pin is tied to GND, so it only talks in the left slot.
        // On the ESP32-S3 the macro above defaults to both slots, so set it explicitly.
        std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

        // Part 3: apply the same config to both handles, then start them.
        // Init both before enabling either, because they share one clock.
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx, &std_cfg));
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx, &std_cfg));
        ESP_ERROR_CHECK(i2s_channel_enable(tx));
        ESP_ERROR_CHECK(i2s_channel_enable(rx));

        // Reaching this line means every call above returned ESP_OK.
        printf("Bring-up start! I2S running at %d Hz\n", SAMPLE_RATE_HZ);

    } //app_main

} //extern "C"
