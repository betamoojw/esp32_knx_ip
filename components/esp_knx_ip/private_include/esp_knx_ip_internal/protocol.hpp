#pragma once

#include "esp_knx_ip/knx_types.h"

#include <cstddef>
#include <cstdint>

namespace esp_knx_ip::internal {

constexpr uint16_t kRoutingIndication = 0x0530;
constexpr uint8_t kLDataIndication = 0x29;

enum class ParseResult {
    kOk,
    kInvalidArgument,
    kTruncated,
    kInvalidHeader,
    kUnsupportedService,
    kUnsupportedMessage,
    kUnsupportedCommand,
    kNotGroupAddressed,
    kInvalidLength,
};

bool build_routing_indication(knx_address_t source, knx_address_t destination,
                              knx_command_t command, const uint8_t *data,
                              size_t data_length, uint8_t *output,
                              size_t output_capacity, size_t *output_length);

ParseResult parse_routing_indication(const uint8_t *packet, size_t packet_length,
                                     knx_telegram_t *telegram);

}  // namespace esp_knx_ip::internal
