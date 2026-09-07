#pragma once

#include "esp_err.h"
#include "esp_knx_ip/knx_types.h"
#include "esp_knx_ip_internal/context.hpp"

#include <stddef.h>
#include <stdint.h>

namespace esp_knx_ip::internal {

esp_err_t transport_open(esp_knx_ip_context *context);
void transport_close(esp_knx_ip_context *context);
esp_err_t transport_start_receive_task(esp_knx_ip_context *context);
void transport_wake_receiver(esp_knx_ip_context *context);
esp_err_t transport_send(esp_knx_ip_context *context,
                         const sockaddr_in &endpoint,
                         knx_address_t group_address,
                         knx_command_t command,
                         const uint8_t *data,
                         size_t data_length);

}  // namespace esp_knx_ip::internal
