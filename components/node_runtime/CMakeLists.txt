idf_component_register(
    SRCS "node_runtime.cpp" "node_state.cpp"
    INCLUDE_DIRS "include"
    REQUIRES config anc_hal esp_timer
)
