idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_event esp_knx_ip esp_wifi nvs_flash
)
