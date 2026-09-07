idf_component_register(
    SRCS
        "src/esp_knx_ip.cpp"
        "src/knx_dpt.cpp"
        "src/knx_ip_config.cpp"
        "src/knx_ip_protocol.cpp"
        "src/knx_ip_transport.cpp"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "private_include"
    REQUIRES esp_netif
    PRIV_REQUIRES nvs_flash
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)