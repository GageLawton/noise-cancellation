#include "anc/i2s_audio_io.hpp"

#include "anc/sample_format.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

namespace anc {
namespace {

constexpr const char* kTag = "i2s_io";

}  // namespace

bool I2sAudioIo::init(const I2sSettings& settings) {
    // One port, master, with a TX handle (amp) and an RX handle (mic).
    // Opening both together makes them share BCLK and WS.
    i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chanCfg.dma_frame_num     = static_cast<uint32_t>(settings.dmaFrames);
    chanCfg.dma_desc_num      = static_cast<uint32_t>(settings.dmaBuffers);
    // If the audio task ever misses a block, the amp plays zeros instead of
    // repeating the last buffer (which would buzz).
    chanCfg.auto_clear = true;
    esp_err_t err      = i2s_new_channel(&chanCfg, &tx_, &rx_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "i2s_new_channel failed: %s", esp_err_to_name(err));
        return false;
    }

    i2s_std_config_t stdCfg = {
        .clk_cfg =
            I2S_STD_CLK_DEFAULT_CONFIG(static_cast<uint32_t>(settings.sampleRateHz)),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                        I2S_SLOT_MODE_MONO),
        .gpio_cfg =
            {
                .mclk         = I2S_GPIO_UNUSED,
                .bclk         = static_cast<gpio_num_t>(settings.bclkPin),
                .ws           = static_cast<gpio_num_t>(settings.wsPin),
                .dout         = static_cast<gpio_num_t>(settings.doutPin),
                .din          = static_cast<gpio_num_t>(settings.dinPin),
                .invert_flags = {},
            },
    };
    // The mic's L/R pin is tied to GND, so it talks in the left slot.
    stdCfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    err = i2s_channel_init_std_mode(tx_, &stdCfg);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "TX init failed: %s", esp_err_to_name(err));
        return false;
    }
    err = i2s_channel_init_std_mode(rx_, &stdCfg);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "RX init failed: %s", esp_err_to_name(err));
        return false;
    }

    err = i2s_channel_enable(tx_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "TX enable failed: %s", esp_err_to_name(err));
        return false;
    }
    err = i2s_channel_enable(rx_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "RX enable failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(kTag, "I2S running at %d Hz, %d DMA buffers of %d samples",
             settings.sampleRateHz, settings.dmaBuffers, settings.dmaFrames);
    return true;
}

std::size_t I2sAudioIo::readBlock(float* buf, std::size_t n) {
    if (n > kMaxBlock)
        n = kMaxBlock;

    // Blocking is correct here: it paces the audio task at the sample rate.
    std::size_t bytesRead = 0;
    esp_err_t err =
        i2s_channel_read(rx_, rawIn_, n * sizeof(int32_t), &bytesRead, portMAX_DELAY);
    if (err != ESP_OK) {
        // Log only the first failure, so a dead mic doesn't flood the console.
        if (!readErrorLogged_) {
            ESP_LOGE(kTag, "mic read failed: %s", esp_err_to_name(err));
            readErrorLogged_ = true;
        }
        return 0;
    }

    const std::size_t count = bytesRead / sizeof(int32_t);
    for (std::size_t i = 0; i < count; ++i) {
        buf[i] = micSampleToFloat(rawIn_[i]);
    }
    return count;
}

std::size_t I2sAudioIo::writeBlock(const float* buf, std::size_t n) {
    if (n > kMaxBlock)
        n = kMaxBlock;

    for (std::size_t i = 0; i < n; ++i) {
        rawOut_[i] = floatToAmpSample(buf[i]);
    }

    std::size_t bytesWritten = 0;
    esp_err_t err = i2s_channel_write(tx_, rawOut_, n * sizeof(int32_t), &bytesWritten,
                                      portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "amp write failed: %s", esp_err_to_name(err));
        return 0;
    }
    return bytesWritten / sizeof(int32_t);
}

}  // namespace anc
