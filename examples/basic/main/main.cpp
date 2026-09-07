#include "esp_knx_ip/esp_knx_ip.h"
#include "esp_knx_ip/knx_dpt.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include <cstring>

namespace {

constexpr char kTag[] = "knx_example";
constexpr EventBits_t kConnectedBit = BIT0;
EventGroupHandle_t connection_events;

void wifi_event(void *, esp_event_base_t event_base, int32_t event_id, void *)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(connection_events, kConnectedBit);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(connection_events, kConnectedBit);
    }
}

esp_netif_t *connect_wifi()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    connection_events = xEventGroupCreate();
    ESP_ERROR_CHECK(connection_events == nullptr ? ESP_ERR_NO_MEM : ESP_OK);

    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(netif == nullptr ? ESP_ERR_NO_MEM : ESP_OK);
    wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init));
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, nullptr));

    wifi_config_t wifi_config = {};
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid),
                 CONFIG_KNX_EXAMPLE_WIFI_SSID,
                 sizeof(wifi_config.sta.ssid) - 1);
    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.password),
                 CONFIG_KNX_EXAMPLE_WIFI_PASSWORD,
                 sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    xEventGroupWaitBits(connection_events, kConnectedBit, pdFALSE, pdTRUE,
                        portMAX_DELAY);
    return netif;
}

void temperature_received(const knx_telegram_t *telegram, void *)
{
    if (telegram->data_length < 3) {
        ESP_LOGW(kTag, "Short DPT9 telegram");
        return;
    }
    float temperature = 0.0f;
    if (knx_dpt9_decode(telegram->data + 1, telegram->data_length - 1,
                        &temperature)) {
        ESP_LOGI(kTag, "DPT9 value %.2f received on %u/%u/%u", temperature,
                 knx_group_main(telegram->destination),
                 knx_group_middle(telegram->destination),
                 knx_group_sub(telegram->destination));
    }
}

}  // namespace

extern "C" void app_main(void)
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(result);

    esp_netif_t *netif = connect_wifi();
    esp_knx_ip_config_t config = ESP_KNX_IP_CONFIG_DEFAULT(netif);
    config.physical_address = knx_physical_address(1, 1, 1);

    esp_knx_ip_handle_t knx = nullptr;
    ESP_ERROR_CHECK(esp_knx_ip_create(&config, &knx));
    knx_address_t temperature_group = 0;
    ESP_ERROR_CHECK(esp_knx_ip_register_persistent_callback(
        knx, "temperature", knx_group_address(1, 2, 3),
        temperature_received, nullptr, &temperature_group));
    ESP_ERROR_CHECK(esp_knx_ip_start(knx));

    uint8_t encoded_temperature[2];
    if (knx_dpt9_encode(21.5f, encoded_temperature,
                        sizeof(encoded_temperature))) {
        const uint8_t apdu[] = {
            0, encoded_temperature[0], encoded_temperature[1]};
        ESP_ERROR_CHECK(esp_knx_ip_send(
            knx, temperature_group, KNX_COMMAND_WRITE, apdu, sizeof(apdu)));
    }

    if (CONFIG_KNX_EXAMPLE_RUN_SECONDS == 0) {
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    } else {
        vTaskDelay(pdMS_TO_TICKS(CONFIG_KNX_EXAMPLE_RUN_SECONDS * 1000));
    }

    ESP_ERROR_CHECK(esp_knx_ip_stop(knx));
    ESP_ERROR_CHECK(esp_knx_ip_destroy(knx));
}
