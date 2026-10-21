idf_component_register(
    SRCS "node_runtime.cpp" "node_state.cpp"
    INCLUDE_DIRS "include"
    REQUIRES config coordinator anc_hal dsp_core calibration alignment esp_timer esp_app_format
)
