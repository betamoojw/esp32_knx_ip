#include "esp_knx_ip_internal/protocol.hpp"

#include <cstring>

namespace esp_knx_ip::internal {
namespace {

constexpr size_t kHeaderLength = 6;
constexpr size_t kCemiHeaderLength = 2;
constexpr size_t kServiceInfoLength = 8;
constexpr size_t kFrameOverhead = kHeaderLength + kCemiHeaderLength + kServiceInfoLength;

uint16_t read_be16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

void write_be16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8);
    data[1] = (uint8_t)(value & 0xffU);
}

}  // namespace

bool build_routing_indication(knx_address_t source, knx_address_t destination,
                              knx_command_t command, const uint8_t *data,
                              size_t data_length, uint8_t *output,
                              size_t output_capacity, size_t *output_length)
{
    if (output == nullptr || output_length == nullptr ||
        data_length == 0 || data_length > ESP_KNX_IP_MAX_APDU_LENGTH ||
        (data == nullptr && data_length != 0) ||
        command > KNX_COMMAND_WRITE) {
        return false;
    }

    const size_t frame_length = kFrameOverhead + data_length;
    if (frame_length > UINT16_MAX || output_capacity < frame_length) {
        return false;
    }

    output[0] = kHeaderLength;
    output[1] = 0x10;
    write_be16(output + 2, kRoutingIndication);
    write_be16(output + 4, (uint16_t)frame_length);
    output[6] = kLDataIndication;
    output[7] = 0;
    output[8] = 0xbc;
    output[9] = 0xe0;
    write_be16(output + 10, source);
    write_be16(output + 12, destination);
    output[14] = (uint8_t)data_length;
    output[15] = (uint8_t)(((uint8_t)command & 0x0cU) >> 2);
    std::memcpy(output + 16, data, data_length);
    output[16] = (uint8_t)((output[16] & 0x3fU) | (((uint8_t)command & 0x03U) << 6));
    *output_length = frame_length;
    return true;
}

ParseResult parse_routing_indication(const uint8_t *packet, size_t packet_length,
                                     knx_telegram_t *telegram)
{
    if (packet == nullptr || telegram == nullptr) {
        return ParseResult::kInvalidArgument;
    }
    if (packet_length < kFrameOverhead) {
        return ParseResult::kTruncated;
    }
    if (packet[0] != kHeaderLength || packet[1] != 0x10) {
        return ParseResult::kInvalidHeader;
    }
    if (read_be16(packet + 2) != kRoutingIndication) {
        return ParseResult::kUnsupportedService;
    }
    if (read_be16(packet + 4) != packet_length) {
        return ParseResult::kInvalidLength;
    }
    if (packet[6] != kLDataIndication) {
        return ParseResult::kUnsupportedMessage;
    }

    const size_t additional_info_length = packet[7];
    const size_t service_offset = kHeaderLength + kCemiHeaderLength + additional_info_length;
    if (service_offset > packet_length || packet_length - service_offset < kServiceInfoLength) {
        return ParseResult::kTruncated;
    }
    const uint8_t *service = packet + service_offset;
    if ((service[1] & 0x80U) == 0) {
        return ParseResult::kNotGroupAddressed;
    }

    const size_t data_length = service[6];
    if (data_length == 0 || data_length > ESP_KNX_IP_MAX_APDU_LENGTH ||
        data_length != packet_length - service_offset - kServiceInfoLength) {
        return ParseResult::kInvalidLength;
    }

    const uint8_t command = (uint8_t)(((service[7] & 0x03U) << 2) |
                                      ((service[8] & 0xc0U) >> 6));
    if (command > KNX_COMMAND_WRITE) {
        return ParseResult::kUnsupportedCommand;
    }

    telegram->source = read_be16(service + 2);
    telegram->destination = read_be16(service + 4);
    telegram->command = (knx_command_t)command;
    telegram->data_length = data_length;
    std::memcpy(telegram->data, service + 8, data_length);
    telegram->data[0] &= 0x3fU;
    return ParseResult::kOk;
}

}  // namespace esp_knx_ip::internal