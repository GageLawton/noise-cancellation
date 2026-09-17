idf_component_register(
    SRCS "node_config.cpp" "config_store.cpp"
    INCLUDE_DIRS "include"
    REQUIRES json joltwallet__littlefs
)
