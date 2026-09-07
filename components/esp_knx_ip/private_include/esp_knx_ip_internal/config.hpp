#pragma once

#include "esp_err.h"
#include "esp_knx_ip/knx_types.h"

namespace esp_knx_ip::internal {

esp_err_t load_physical_address(knx_address_t *address);
esp_err_t save_physical_address(knx_address_t address);
esp_err_t load_group_address(const char *name, knx_address_t *address);
esp_err_t save_group_address(const char *name, knx_address_t address);

}  // namespace esp_knx_ip::internal
