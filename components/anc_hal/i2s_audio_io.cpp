#include "anc/i2s_audio_io.hpp"

#include "esp_log.h"

namespace anc {
namespace {

constexpr const char* kTag = "i2s_io";

}  // namespace

bool I2sAudioIo::init(const I2sSettings&) {
    ESP_LOGI(kTag, "I2S init (stub)");
    return true;
}

std::size_t I2sAudioIo::readBlock(float*, std::size_t) {
    return 0;
}

std::size_t I2sAudioIo::writeBlock(const float*, std::size_t) {
    return 0;
}

}  // namespace anc
