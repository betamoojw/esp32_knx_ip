#pragma once

#include "esp_knx_ip/esp_knx_ip.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "sdkconfig.h"

#ifndef CONFIG_ESP_KNX_IP_MAX_GROUP_ADDRESSES
#define CONFIG_ESP_KNX_IP_MAX_GROUP_ADDRESSES 32
#endif

struct esp_knx_ip_callback_registration {
    bool used;
    knx_address_t group_address;
    esp_knx_ip_telegram_callback_t callback;
    void *user_context;
    char mapping_name[ESP_KNX_IP_GROUP_NAME_MAX_LENGTH + 1];
};

struct esp_knx_ip_context {
    esp_netif_t *netif = nullptr;
    knx_address_t physical_address = 0;
    char multicast_address[INET_ADDRSTRLEN] = {};
    uint16_t port = 0;
    int socket = -1;
    sockaddr_in multicast_endpoint = {};
    ip_mreq multicast_membership = {};
    SemaphoreHandle_t mutex = nullptr;
    EventGroupHandle_t events = nullptr;
    TaskHandle_t task = nullptr;
    esp_knx_ip_state_t state = ESP_KNX_IP_STATE_INITIALIZED;
    esp_knx_ip_callback_registration registrations[CONFIG_ESP_KNX_IP_MAX_GROUP_ADDRESSES] = {};
};

namespace esp_knx_ip::internal {

constexpr EventBits_t kTaskStoppedBit = BIT0;

inline void lock(esp_knx_ip_context *context)
{
    xSemaphoreTake(context->mutex, portMAX_DELAY);
}

inline void unlock(esp_knx_ip_context *context)
{
    xSemaphoreGive(context->mutex);
}

void dispatch_telegram(esp_knx_ip_context *context,
                       const knx_telegram_t &telegram);

}  // namespace esp_knx_ip::internal
