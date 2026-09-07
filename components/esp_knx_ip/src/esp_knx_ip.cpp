#include "esp_knx_ip/esp_knx_ip.h"

#include "esp_knx_ip_internal/config.hpp"
#include "esp_knx_ip_internal/context.hpp"
#include "esp_knx_ip_internal/transport.hpp"

#include "esp_log.h"
#include "lwip/inet.h"
#include "nvs.h"

#include <cstring>
#include <new>

namespace {

constexpr char kTag[] = "esp_knx_ip";

}  // namespace

namespace esp_knx_ip::internal {

void dispatch_telegram(esp_knx_ip_context *context,
                       const knx_telegram_t &telegram)
{
    esp_knx_ip_telegram_callback_t callback = nullptr;
    void *user_context = nullptr;

    lock(context);
    for (const esp_knx_ip_callback_registration &registration :
         context->registrations) {
        if (registration.used &&
            registration.group_address == telegram.destination) {
            callback = registration.callback;
            user_context = registration.user_context;
            break;
        }
    }
    unlock(context);

    if (callback != nullptr) {
        callback(&telegram, user_context);
    }
}

}  // namespace esp_knx_ip::internal

using esp_knx_ip::internal::lock;
using esp_knx_ip::internal::transport_close;
using esp_knx_ip::internal::transport_open;
using esp_knx_ip::internal::transport_send;
using esp_knx_ip::internal::transport_start_receive_task;
using esp_knx_ip::internal::transport_wake_receiver;
using esp_knx_ip::internal::unlock;

extern "C" esp_err_t esp_knx_ip_create(const esp_knx_ip_config_t *config,
                                        esp_knx_ip_handle_t *out_handle)
{
    if (config == nullptr || out_handle == nullptr || config->netif == nullptr ||
        config->multicast_address == nullptr || config->port == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_handle = nullptr;

    auto *context = new (std::nothrow) esp_knx_ip_context;
    if (context == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    context->mutex = xSemaphoreCreateMutex();
    context->events = xEventGroupCreate();
    if (context->mutex == nullptr || context->events == nullptr) {
        if (context->mutex != nullptr) {
            vSemaphoreDelete(context->mutex);
        }
        if (context->events != nullptr) {
            vEventGroupDelete(context->events);
        }
        delete context;
        return ESP_ERR_NO_MEM;
    }
    if (std::strlen(config->multicast_address) >=
            sizeof(context->multicast_address) ||
        inet_aton(config->multicast_address,
                  &context->multicast_endpoint.sin_addr) == 0) {
        vSemaphoreDelete(context->mutex);
        vEventGroupDelete(context->events);
        delete context;
        return ESP_ERR_INVALID_ARG;
    }

    context->netif = config->netif;
    context->physical_address = config->physical_address;
    context->port = config->port;
    std::strcpy(context->multicast_address, config->multicast_address);

    if (config->load_physical_address_from_nvs) {
        knx_address_t saved_address = 0;
        const esp_err_t result =
            esp_knx_ip::internal::load_physical_address(&saved_address);
        if (result == ESP_OK) {
            context->physical_address = saved_address;
        } else if (result != ESP_ERR_NVS_NOT_FOUND &&
                   result != ESP_ERR_NVS_NOT_INITIALIZED) {
            ESP_LOGW(kTag, "Could not load KNX address from NVS: %s",
                     esp_err_to_name(result));
        }
    }

    *out_handle = context;
    return ESP_OK;
}

extern "C" esp_err_t esp_knx_ip_destroy(esp_knx_ip_handle_t handle)
{
    if (handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    lock(handle);
    const bool active = handle->state == ESP_KNX_IP_STATE_RUNNING ||
                        handle->task != nullptr;
    unlock(handle);
    if (active) {
        return ESP_ERR_INVALID_STATE;
    }
    transport_close(handle);
    vSemaphoreDelete(handle->mutex);
    vEventGroupDelete(handle->events);
    delete handle;
    return ESP_OK;
}

extern "C" esp_err_t esp_knx_ip_start(esp_knx_ip_handle_t handle)
{
    if (handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    lock(handle);
    if (handle->state == ESP_KNX_IP_STATE_RUNNING) {
        unlock(handle);
        return ESP_ERR_INVALID_STATE;
    }
    xEventGroupClearBits(handle->events,
                         esp_knx_ip::internal::kTaskStoppedBit);
    esp_err_t result = transport_open(handle);
    if (result != ESP_OK) {
        unlock(handle);
        return result;
    }
    handle->state = ESP_KNX_IP_STATE_RUNNING;
    result = transport_start_receive_task(handle);
    if (result != ESP_OK) {
        handle->state = ESP_KNX_IP_STATE_STOPPED;
        transport_close(handle);
        unlock(handle);
        return result;
    }
    unlock(handle);
    ESP_LOGI(kTag, "Joined %s:%u", handle->multicast_address, handle->port);
    return ESP_OK;
}

extern "C" esp_err_t esp_knx_ip_stop(esp_knx_ip_handle_t handle)
{
    if (handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    lock(handle);
    if (xTaskGetCurrentTaskHandle() == handle->task) {
        unlock(handle);
        return ESP_ERR_INVALID_STATE;
    }
    if (handle->state != ESP_KNX_IP_STATE_RUNNING) {
        unlock(handle);
        return ESP_ERR_INVALID_STATE;
    }
    handle->state = ESP_KNX_IP_STATE_STOPPED;
    transport_wake_receiver(handle);
    unlock(handle);

    xEventGroupWaitBits(handle->events,
                        esp_knx_ip::internal::kTaskStoppedBit,
                        pdTRUE, pdTRUE, portMAX_DELAY);
    lock(handle);
    handle->task = nullptr;
    transport_close(handle);
    unlock(handle);
    return ESP_OK;
}

extern "C" esp_err_t esp_knx_ip_get_state(esp_knx_ip_handle_t handle,
                                           esp_knx_ip_state_t *state)
{
    if (handle == nullptr || state == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    lock(handle);
    *state = handle->state;
    unlock(handle);
    return ESP_OK;
}

extern "C" esp_err_t esp_knx_ip_set_physical_address(
    esp_knx_ip_handle_t handle, knx_address_t address, bool persist)
{
    if (handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (persist) {
        const esp_err_t result =
            esp_knx_ip::internal::save_physical_address(address);
        if (result != ESP_OK) {
            return result;
        }
    }
    lock(handle);
    handle->physical_address = address;
    unlock(handle);
    return ESP_OK;
}

extern "C" esp_err_t esp_knx_ip_get_physical_address(
    esp_knx_ip_handle_t handle, knx_address_t *address)
{
    if (handle == nullptr || address == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    lock(handle);
    *address = handle->physical_address;
    unlock(handle);
    return ESP_OK;
}

extern "C" esp_err_t esp_knx_ip_register_callback(
    esp_knx_ip_handle_t handle, knx_address_t group_address,
    esp_knx_ip_telegram_callback_t callback, void *user_context)
{
    if (handle == nullptr || callback == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    lock(handle);
    esp_knx_ip_callback_registration *free_registration = nullptr;
    for (esp_knx_ip_callback_registration &registration :
         handle->registrations) {
        if (registration.used &&
            registration.group_address == group_address) {
            registration.callback = callback;
            registration.user_context = user_context;
            unlock(handle);
            return ESP_OK;
        }
        if (!registration.used && free_registration == nullptr) {
            free_registration = &registration;
        }
    }
    if (free_registration == nullptr) {
        unlock(handle);
        return ESP_ERR_NO_MEM;
    }
    free_registration->used = true;
    free_registration->group_address = group_address;
    free_registration->callback = callback;
    free_registration->user_context = user_context;
    free_registration->mapping_name[0] = '\0';
    unlock(handle);
    return ESP_OK;
}

extern "C" esp_err_t esp_knx_ip_unregister_callback(
    esp_knx_ip_handle_t handle, knx_address_t group_address)
{
    if (handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    lock(handle);
    for (esp_knx_ip_callback_registration &registration :
         handle->registrations) {
        if (registration.used &&
            registration.group_address == group_address) {
            registration = {};
            unlock(handle);
            return ESP_OK;
        }
    }
    unlock(handle);
    return ESP_ERR_NOT_FOUND;
}

extern "C" esp_err_t esp_knx_ip_register_persistent_callback(
    esp_knx_ip_handle_t handle, const char *mapping_name,
    knx_address_t default_group_address,
    esp_knx_ip_telegram_callback_t callback, void *user_context,
    knx_address_t *resolved_group_address)
{
    if (handle == nullptr || mapping_name == nullptr || callback == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t name_length = std::strlen(mapping_name);
    if (name_length == 0 || name_length > ESP_KNX_IP_GROUP_NAME_MAX_LENGTH) {
        return ESP_ERR_INVALID_ARG;
    }

    knx_address_t address = default_group_address;
    const esp_err_t load_result =
        esp_knx_ip::internal::load_group_address(mapping_name, &address);
    if (load_result != ESP_OK && load_result != ESP_ERR_NVS_NOT_FOUND) {
        return load_result;
    }

    lock(handle);
    esp_knx_ip_callback_registration *target = nullptr;
    esp_knx_ip_callback_registration *free_registration = nullptr;
    for (esp_knx_ip_callback_registration &registration :
         handle->registrations) {
        if (registration.used &&
            (std::strcmp(registration.mapping_name, mapping_name) == 0 ||
             registration.group_address == address)) {
            target = &registration;
            break;
        }
        if (!registration.used && free_registration == nullptr) {
            free_registration = &registration;
        }
    }
    if (target == nullptr) {
        target = free_registration;
    }
    if (target == nullptr) {
        unlock(handle);
        return ESP_ERR_NO_MEM;
    }
    target->used = true;
    target->group_address = address;
    target->callback = callback;
    target->user_context = user_context;
    std::strcpy(target->mapping_name, mapping_name);
    unlock(handle);

    if (resolved_group_address != nullptr) {
        *resolved_group_address = address;
    }
    return ESP_OK;
}

extern "C" esp_err_t esp_knx_ip_set_registered_group_address(
    esp_knx_ip_handle_t handle, const char *mapping_name,
    knx_address_t group_address, bool persist)
{
    if (handle == nullptr || mapping_name == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t name_length = std::strlen(mapping_name);
    if (name_length == 0 || name_length > ESP_KNX_IP_GROUP_NAME_MAX_LENGTH) {
        return ESP_ERR_INVALID_ARG;
    }
    if (persist) {
        const esp_err_t result = esp_knx_ip::internal::save_group_address(
            mapping_name, group_address);
        if (result != ESP_OK) {
            return result;
        }
    }

    lock(handle);
    for (esp_knx_ip_callback_registration &registration :
         handle->registrations) {
        if (registration.used &&
            std::strcmp(registration.mapping_name, mapping_name) == 0) {
            registration.group_address = group_address;
            unlock(handle);
            return ESP_OK;
        }
    }
    unlock(handle);
    return ESP_ERR_NOT_FOUND;
}

extern "C" esp_err_t esp_knx_ip_send(esp_knx_ip_handle_t handle,
                                      knx_address_t group_address,
                                      knx_command_t command,
                                      const uint8_t *data,
                                      size_t data_length)
{
    if (handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    return transport_send(handle, handle->multicast_endpoint, group_address,
                          command, data, data_length);
}

extern "C" esp_err_t esp_knx_ip_send_unicast(
    esp_knx_ip_handle_t handle, const char *destination_ipv4,
    uint16_t destination_port, knx_address_t group_address,
    knx_command_t command, const uint8_t *data, size_t data_length)
{
    if (handle == nullptr || destination_ipv4 == nullptr ||
        destination_port == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    sockaddr_in endpoint = {};
    endpoint.sin_family = AF_INET;
    endpoint.sin_port = htons(destination_port);
    if (inet_aton(destination_ipv4, &endpoint.sin_addr) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return transport_send(handle, endpoint, group_address, command, data,
                          data_length);
}
