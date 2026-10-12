#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

// Breadboard wiring (see hardware/breadboard-wiring guide).
// The mic and the amp share BCLK and WS, so both sides run off one clock.
#define PIN_BCLK GPIO_NUM_4   // bit clock: mic SCK + amp BCLK
#define PIN_WS   GPIO_NUM_5   // word select (left/right): mic WS + amp LRC
#define PIN_DOUT GPIO_NUM_6   // ESP32 -> amp DIN (speaker data)
#define PIN_DIN  GPIO_NUM_7   // mic SD -> ESP32 (microphone data)

// Settings you can change without editing code: run `idf.py menuconfig` and open
// "I2S bring-up". The defaults live in main/Kconfig.projbuild.
#define SAMPLE_RATE_HZ     CONFIG_BRINGUP_SAMPLE_RATE_HZ
#define NOISE_FLOOR_DBFS   CONFIG_BRINGUP_NOISE_FLOOR_DBFS  // placeholder until measured

// Step 3: read the mic in blocks of 512 samples (32 ms at 16 kHz) and print the
// level once every 8 blocks, which is about 4 times a second.
#define BLOCK_SAMPLES     512
#define BLOCKS_PER_PRINT  8

// Lives outside app_main so the 2 KB buffer doesn't eat the main task's small stack.
static int32_t mic_block[BLOCK_SAMPLES];

// The INMP441 sends a 24-bit sample left-aligned in a 32-bit slot (the low 8 bits are 0).
// Shifting right by 8 gives the signed 24-bit value, and dividing by 2^23 scales it
// to -1.0 .. +1.0, where 1.0 is the loudest the mic can report ("full scale").
static float mic_sample_to_float(int32_t raw)
{
    return (float)(raw >> 8) / 8388608.0f;
}

#ifdef CONFIG_BRINGUP_TONE_ENABLE
// Step 4: tone out. A separate FreeRTOS task keeps the speaker fed while app_main
// keeps reading the mic, so both directions of the bus run at the same time.
#define TONE_BLOCK_SAMPLES 256
#define TONE_AMPLITUDE     (CONFIG_BRINGUP_TONE_AMPLITUDE_PCT / 100.0f)

#ifdef CONFIG_BRINGUP_TONE_CYCLE
// 0 means silence, so each run also records the mic with the speaker quiet.
static const int tone_cycle_hz[] = {0, 60, 120, 180, 440};
#define TONE_CYCLE_STEPS (sizeof(tone_cycle_hz) / sizeof(tone_cycle_hz[0]))
#define TONE_STEP_BLOCKS (CONFIG_BRINGUP_TONE_STEP_SECONDS * SAMPLE_RATE_HZ / TONE_BLOCK_SAMPLES)
#endif

static int32_t tone_block[TONE_BLOCK_SAMPLES];

// The tone frequency currently playing, so the mic loop can label its readings.
// One task writes it and the other only reads it, and a 32-bit write is atomic
// on the ESP32, so volatile is enough here.
static volatile int current_tone_hz = 0;

static void tone_task(void *arg)
{
    i2s_chan_handle_t tx = (i2s_chan_handle_t)arg;

    // Phase runs from 0 to 1 once per sine cycle. Keeping it between blocks makes
    // the wave continuous, so there's no click every time a new block starts.
    float phase = 0.0f;
#ifdef CONFIG_BRINGUP_TONE_CYCLE
    size_t step = 0;
    int blocks_in_step = 0;
    current_tone_hz = tone_cycle_hz[0];
#else
    current_tone_hz = CONFIG_BRINGUP_TONE_FREQ_HZ;
#endif

    while (true) {
        const int hz = current_tone_hz;
        const float phase_step = (float)hz / SAMPLE_RATE_HZ;
        for (int i = 0; i < TONE_BLOCK_SAMPLES; i++) {
            float x = TONE_AMPLITUDE * sinf(2.0f * (float)M_PI * phase);
            // Float -1..+1 to the amp's 32-bit format: scale by the largest int32.
            tone_block[i] = (int32_t)(x * 2147483647.0f);
            phase += phase_step;
            if (phase >= 1.0f) {
                phase -= 1.0f;
            }
        }

        // Blocks until the DMA has room, which paces this loop at exactly the sample rate.
        size_t written = 0;
        ESP_ERROR_CHECK(i2s_channel_write(tx, tone_block, sizeof(tone_block), &written, portMAX_DELAY));

#ifdef CONFIG_BRINGUP_TONE_CYCLE
        if (++blocks_in_step >= TONE_STEP_BLOCKS) {
            blocks_in_step = 0;
            step = (step + 1) % TONE_CYCLE_STEPS;
            current_tone_hz = tone_cycle_hz[step];
            if (current_tone_hz == 0) {
                printf("--- tone: off ---\n");
            } else {
                printf("--- tone: %d Hz ---\n", current_tone_hz);
            }
        }
#endif
    }
}
#endif  // CONFIG_BRINGUP_TONE_ENABLE

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
        // The clock and pins are shared; the slot mask is set per direction below.
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
        // The two directions differ only in which slots they use.
        // Speaker: on the ESP32-S3, mono with BOTH slots copies each sample into the left
        // and the right slot. Many MAX98357A boards play (L+R)/2, so with only the left
        // slot filled the tone would come out at half amplitude (about 6 dB quieter).
        i2s_std_config_t tx_cfg = std_cfg;
        tx_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
        // Mic: its L/R pin is tied to GND, so it only talks in the left slot.
        i2s_std_config_t rx_cfg = std_cfg;
        rx_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

        // Part 3: apply the configs to both handles, then start them.
        // Init both before enabling either, because they share one clock.
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx, &tx_cfg));
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx, &rx_cfg));
        ESP_ERROR_CHECK(i2s_channel_enable(tx));
        ESP_ERROR_CHECK(i2s_channel_enable(rx));

        // Reaching this line means every call above returned ESP_OK.
        printf("Bring-up start! I2S running at %d Hz, noise floor set to %d dBFS\n",
               SAMPLE_RATE_HZ, NOISE_FLOOR_DBFS);

#ifdef CONFIG_BRINGUP_TONE_ENABLE
        // Start the speaker task. 4 KB of stack is plenty since its buffer is static.
        xTaskCreate(tone_task, "tone", 4096, tx, 5, NULL);
#endif

        // Step 3: mic level meter. app_main never returns from here.
        double sum_squares = 0.0;
        int blocks = 0;
        while (true) {
            // Blocks until 512 samples have arrived from the mic (about 32 ms).
            size_t bytes_read = 0;
            ESP_ERROR_CHECK(i2s_channel_read(rx, mic_block, sizeof(mic_block), &bytes_read, portMAX_DELAY));
            size_t n = bytes_read / sizeof(int32_t);
            if (n == 0) {
                continue;
            }

            // Many INMP441 boards add a small constant offset (DC). Subtract the block's
            // average so the meter shows sound, not that offset.
            float mean = 0.0f;
            for (size_t i = 0; i < n; i++) {
                mean += mic_sample_to_float(mic_block[i]);
            }
            mean /= (float)n;

            for (size_t i = 0; i < n; i++) {
                float x = mic_sample_to_float(mic_block[i]) - mean;
                sum_squares += (double)x * x;
            }

            if (++blocks == BLOCKS_PER_PRINT) {
                // RMS = square root of the average squared sample: the "loudness" of the signal.
                // dBFS = 20 * log10(rms): 0 dBFS is full scale, quieter is more negative.
                // Rough guide for an INMP441: quiet room about -75 to -85, talking nearby
                // about -50 to -40, a clap close by -20 or louder.
                double rms = sqrt(sum_squares / (BLOCKS_PER_PRINT * (double)n));
                double dbfs = (rms > 1e-9) ? 20.0 * log10(rms) : -180.0;  // log10(0) is -infinity
                // raw[0] is one untouched sample, for debugging wiring: all zeros or a value
                // that never changes means no data is arriving on GPIO7.
#ifdef CONFIG_BRINGUP_TONE_ENABLE
                const int tone_hz = current_tone_hz;
#else
                const int tone_hz = 0;
#endif
                printf("mic: %7.1f dBFS  (%+5.1f dB above floor)  tone %3d Hz  (raw[0]=0x%08lx)\n",
                       dbfs, dbfs - NOISE_FLOOR_DBFS, tone_hz, (unsigned long)mic_block[0]);
                sum_squares = 0.0;
                blocks = 0;
            }
        }

    } //app_main

} //extern "C"
