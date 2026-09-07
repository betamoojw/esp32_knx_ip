idf_component_register(
    SRCS "src/esp_knx_ip_webserver.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server esp_knx_ip
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
