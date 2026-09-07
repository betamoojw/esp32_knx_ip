idf_component_register(
    SRCS "test_esp_knx_ip.cpp"
    INCLUDE_DIRS "../private_include"
    PRIV_REQUIRES unity esp_knx_ip nvs_flash
)