idf_component_register(
    SRCS "i2s_audio_io.cpp"
    INCLUDE_DIRS "include"
    REQUIRES config driver esp_driver_i2s log
)
