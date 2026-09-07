#pragma once

#include "esp_err.h"
#include "esp_netif.h"
#include "esp_knx_ip/knx_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_KNX_IP_DEFAULT_MULTICAST_ADDRESS "224.0.23.12"
#define ESP_KNX_IP_DEFAULT_PORT 3671U
#define ESP_KNX_IP_GROUP_NAME_MAX_LENGTH 13U

typedef struct esp_knx_ip_context *esp_knx_ip_handle_t;

typedef enum {
    ESP_KNX_IP_STATE_INITIALIZED,
    ESP_KNX_IP_STATE_RUNNING,
    ESP_KNX_IP_STATE_STOPPED,
} esp_knx_ip_state_t;

typedef struct {
    esp_netif_t *netif;
    knx_address_t physical_address;
    const char *multicast_address;
    uint16_t port;
    bool load_physical_address_from_nvs;
} esp_knx_ip_config_t;

typedef void (*esp_knx_ip_telegram_callback_t)(const knx_telegram_t *telegram,
                                               void *user_context);

#define ESP_KNX_IP_CONFIG_DEFAULT(netif_handle) \
    { (netif_handle), 0x1101U, ESP_KNX_IP_DEFAULT_MULTICAST_ADDRESS, \
      ESP_KNX_IP_DEFAULT_PORT, true }

esp_err_t esp_knx_ip_create(const esp_knx_ip_config_t *config,
                            esp_knx_ip_handle_t *out_handle);
esp_err_t esp_knx_ip_destroy(esp_knx_ip_handle_t handle);

esp_err_t esp_knx_ip_start(esp_knx_ip_handle_t handle);
esp_err_t esp_knx_ip_stop(esp_knx_ip_handle_t handle);
esp_err_t esp_knx_ip_get_state(esp_knx_ip_handle_t handle,
                               esp_knx_ip_state_t *state);

esp_err_t esp_knx_ip_set_physical_address(esp_knx_ip_handle_t handle,
                                          knx_address_t address,
                                          bool persist);
esp_err_t esp_knx_ip_get_physical_address(esp_knx_ip_handle_t handle,
                                          knx_address_t *address);

esp_err_t esp_knx_ip_register_callback(esp_knx_ip_handle_t handle,
                                       knx_address_t group_address,
                                       esp_knx_ip_telegram_callback_t callback,
                                       void *user_context);
esp_err_t esp_knx_ip_unregister_callback(esp_knx_ip_handle_t handle,
                                         knx_address_t group_address);
esp_err_t esp_knx_ip_register_persistent_callback(
    esp_knx_ip_handle_t handle, const char *mapping_name,
    knx_address_t default_group_address,
    esp_knx_ip_telegram_callback_t callback, void *user_context,
    knx_address_t *resolved_group_address);
esp_err_t esp_knx_ip_set_registered_group_address(
    esp_knx_ip_handle_t handle, const char *mapping_name,
    knx_address_t group_address, bool persist);

esp_err_t esp_knx_ip_send(esp_knx_ip_handle_t handle,
                          knx_address_t group_address,
                          knx_command_t command,
                          const uint8_t *data,
                          size_t data_length);
esp_err_t esp_knx_ip_send_unicast(esp_knx_ip_handle_t handle,
                                  const char *destination_ipv4,
                                  uint16_t destination_port,
                                  knx_address_t group_address,
                                  knx_command_t command,
                                  const uint8_t *data,
                                  size_t data_length);

#ifdef __cplusplus
}
#endif