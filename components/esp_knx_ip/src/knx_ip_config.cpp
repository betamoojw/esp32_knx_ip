#include "esp_knx_ip_internal/config.hpp"

#include "esp_knx_ip/esp_knx_ip.h"

#include "nvs.h"

#include <cstdio>
#include <cstring>

namespace esp_knx_ip::internal {
namespace {

constexpr char kNamespace[] = "esp_knx_ip";
constexpr char kVersionKey[] = "version";
constexpr char kPhysicalAddressKey[] = "phys_addr";
constexpr uint8_t kConfigVersion = 1;

esp_err_t make_group_key(const char *name, char key[16])
{
    if (name == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t length = std::strlen(name);
    if (length == 0 || length > ESP_KNX_IP_GROUP_NAME_MAX_LENGTH) {
        return ESP_ERR_INVALID_ARG;
    }
    std::snprintf(key, 16, "g_%s", name);
    return ESP_OK;
}

}  // namespace

esp_err_t load_physical_address(knx_address_t *address)
{
    if (address == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t result = nvs_open(kNamespace, NVS_READONLY, &nvs_handle);
    if (result != ESP_OK) {
        return result;
    }

    uint8_t version = 0;
    result = nvs_get_u8(nvs_handle, kVersionKey, &version);
    if (result == ESP_OK && version != kConfigVersion) {
        result = ESP_ERR_INVALID_VERSION;
    }
    if (result == ESP_OK) {
        result = nvs_get_u16(nvs_handle, kPhysicalAddressKey, address);
    }
    nvs_close(nvs_handle);
    return result;
}

esp_err_t save_physical_address(knx_address_t address)
{
    nvs_handle_t nvs_handle;
    esp_err_t result = nvs_open(kNamespace, NVS_READWRITE, &nvs_handle);
    if (result != ESP_OK) {
        return result;
    }

    result = nvs_set_u8(nvs_handle, kVersionKey, kConfigVersion);
    if (result == ESP_OK) {
        result = nvs_set_u16(nvs_handle, kPhysicalAddressKey, address);
    }
    if (result == ESP_OK) {
        result = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);
    return result;
}

esp_err_t load_group_address(const char *name, knx_address_t *address)
{
    if (address == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    char key[16];
    esp_err_t result = make_group_key(name, key);
    if (result != ESP_OK) {
        return result;
    }
    nvs_handle_t nvs_handle;
    result = nvs_open(kNamespace, NVS_READONLY, &nvs_handle);
    if (result == ESP_OK) {
        uint8_t version = 0;
        result = nvs_get_u8(nvs_handle, kVersionKey, &version);
        if (result == ESP_OK && version != kConfigVersion) {
            result = ESP_ERR_INVALID_VERSION;
        }
        if (result == ESP_OK) {
            result = nvs_get_u16(nvs_handle, key, address);
        }
        nvs_close(nvs_handle);
    }
    return result;
}

esp_err_t save_group_address(const char *name, knx_address_t address)
{
    char key[16];
    esp_err_t result = make_group_key(name, key);
    if (result != ESP_OK) {
        return result;
    }
    nvs_handle_t nvs_handle;
    result = nvs_open(kNamespace, NVS_READWRITE, &nvs_handle);
    if (result == ESP_OK) {
        result = nvs_set_u8(nvs_handle, kVersionKey, kConfigVersion);
        if (result == ESP_OK) {
            result = nvs_set_u16(nvs_handle, key, address);
        }
        if (result == ESP_OK) {
            result = nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
    }
    return result;
}

}  // namespace esp_knx_ip::internal