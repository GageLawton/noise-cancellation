#include "anc/i2s_audio_io.hpp"

#include "esp_log.h"

namespace anc {
namespace {

constexpr const char* kTag = "i2s_io";

}  // namespace

bool I2sAudioIo::init(const I2sSettings& settings) {
    // One port, master, with a TX handle (amp) and an RX handle (mic).
    // Opening both together makes them share BCLK and WS.
    i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chanCfg.dma_frame_num     = static_cast<uint32_t>(settings.dmaFrames);
    esp_err_t err             = i2s_new_channel(&chanCfg, &tx_, &rx_);
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

    ESP_LOGI(kTag, "I2S running at %d Hz, %d-sample DMA blocks", settings.sampleRateHz,
             settings.dmaFrames);
    return true;
}

std::size_t I2sAudioIo::readBlock(float*, std::size_t) {
    return 0;
}

std::size_t I2sAudioIo::writeBlock(const float*, std::size_t) {
    return 0;
}

}  // namespace anc
