#include <stdio.h>
#include "driver/i2s_std.h"

extern "C" {

    void app_main() {

        i2s_std_config_t chan_i2s_chan_config_tcfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        i2s_chan_handle_t tx, rx;
        i2s_new_channel(&chan_i2s_chan_config_tcfg, &tx, &rx);

        printf("Bring-up start!\n");

    } //app_main


} //extern "C"
