#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_KNX_IP_MAX_APDU_LENGTH 255U

typedef uint16_t knx_address_t;

typedef enum {
    KNX_COMMAND_READ = 0x00,
    KNX_COMMAND_RESPONSE = 0x01,
    KNX_COMMAND_WRITE = 0x02,
} knx_command_t;

typedef struct {
    knx_command_t command;
    knx_address_t source;
    knx_address_t destination;
    size_t data_length;
    uint8_t data[ESP_KNX_IP_MAX_APDU_LENGTH];
} knx_telegram_t;

static inline knx_address_t knx_group_address(uint8_t main, uint8_t middle, uint8_t sub)
{
    return (knx_address_t)((((uint16_t)main & 0x1fU) << 11) |
                           (((uint16_t)middle & 0x07U) << 8) |
                           (uint16_t)sub);
}

static inline knx_address_t knx_physical_address(uint8_t area, uint8_t line, uint8_t member)
{
    return (knx_address_t)((((uint16_t)area & 0x0fU) << 12) |
                           (((uint16_t)line & 0x0fU) << 8) |
                           (uint16_t)member);
}

static inline uint8_t knx_group_main(knx_address_t address) { return (uint8_t)((address >> 11) & 0x1fU); }
static inline uint8_t knx_group_middle(knx_address_t address) { return (uint8_t)((address >> 8) & 0x07U); }
static inline uint8_t knx_group_sub(knx_address_t address) { return (uint8_t)(address & 0xffU); }
static inline uint8_t knx_physical_area(knx_address_t address) { return (uint8_t)((address >> 12) & 0x0fU); }
static inline uint8_t knx_physical_line(knx_address_t address) { return (uint8_t)((address >> 8) & 0x0fU); }
static inline uint8_t knx_physical_member(knx_address_t address) { return (uint8_t)(address & 0xffU); }

#ifdef __cplusplus
}
#endif