#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_knx_ip/esp_knx_ip.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_KNX_IP_WEB_CONFIG_URI "/knx/config"

esp_err_t esp_knx_ip_web_register(httpd_handle_t server,
                                  esp_knx_ip_handle_t knx_handle);
esp_err_t esp_knx_ip_web_unregister(httpd_handle_t server);

#ifdef __cplusplus
}
#endif
