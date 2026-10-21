#include "anc/gpio_sync_tick.hpp"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_rom_sys.h"

namespace anc {

bool GpioSyncTick::begin(int outPin, int inPin) {
    outPin_ = outPin;
    inPin_  = inPin;

    gpio_config_t out{};
    out.pin_bit_mask = 1ULL << outPin;
    out.mode         = GPIO_MODE_OUTPUT;
    out.intr_type    = GPIO_INTR_DISABLE;
    if (gpio_config(&out) != ESP_OK)
        return false;
    if (gpio_set_level(static_cast<gpio_num_t>(outPin), 0) != ESP_OK)
        return false;

    gpio_config_t in{};
    in.pin_bit_mask = 1ULL << inPin;
    in.mode         = GPIO_MODE_INPUT;
    in.intr_type    = GPIO_INTR_POSEDGE;
    if (gpio_config(&in) != ESP_OK)
        return false;

    if (arrived_ == nullptr) {
        arrived_ = xSemaphoreCreateBinary();
        if (arrived_ == nullptr)
            return false;
    }

    // Another driver may have installed the shared ISR service already;
    // that's fine, we only need it to exist.
    const esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        return false;
    return gpio_isr_handler_add(static_cast<gpio_num_t>(inPin), &GpioSyncTick::onEdge,
                                this) == ESP_OK;
}

void GpioSyncTick::signal() {
    gpio_set_level(static_cast<gpio_num_t>(outPin_), 1);
    esp_rom_delay_us(10);
    gpio_set_level(static_cast<gpio_num_t>(outPin_), 0);
}

bool GpioSyncTick::wait(uint32_t timeoutMs) {
    if (arrived_ == nullptr)
        return false;
    return xSemaphoreTake(arrived_, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void IRAM_ATTR GpioSyncTick::onEdge(void* arg) {
    auto* self             = static_cast<GpioSyncTick*>(arg);
    BaseType_t higherWoken = pdFALSE;
    xSemaphoreGiveFromISR(self->arrived_, &higherWoken);
    portYIELD_FROM_ISR(higherWoken);
}

}  // namespace anc
