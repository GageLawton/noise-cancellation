#include <stdio.h>
#include "driver/i2s_std.h"
#include "esp_err.h"

extern "C" {

    void app_main() {

        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        i2s_chan_handle_t tx, rx;
        i2s_new_channel(&chan_cfg, &tx, &rx);

        // wrap this in a try-catch block to handle errors
        try {
            i2s_new_channel(&chan_cfg, &tx, &rx);
        } catch (const std::exception& e) {
            printf("Error creating I2S channel: %s\n", e.what());
        } catch (...) {
            printf("Unknown error creating I2S channel\n");
        }

        //the i2s_std_config_config_t block from the skeleton, plus the slot_mask line right after it
        i2s_std_config_t std_cfg = I2S_STD_CONFIG_DEFAULT();
        std_cfg.slot_mask = I2S_SLOT_MASK_0;   
        ESP_ERROR_CHECK(init_std_mode(tx));
        ESP_ERROR_CHECK(init_std_mode(rx));
        ESP_ERROR_CHECK(enable(rx));
        ESP_ERROR_CHECK(enable(tx));

        printf("Bring-up start!\n");

    } //app_main

} //extern "C"
