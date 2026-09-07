#include "esp_knx_ip/knx_dpt.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace {

uint16_t read_be16(const uint8_t *data)
{
    return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
}

uint32_t read_be32(const uint8_t *data)
{
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) | data[3];
}

void write_be16(uint16_t value, uint8_t *data)
{
    data[0] = static_cast<uint8_t>(value >> 8);
    data[1] = static_cast<uint8_t>(value);
}

void write_be32(uint32_t value, uint8_t *data)
{
    data[0] = static_cast<uint8_t>(value >> 24);
    data[1] = static_cast<uint8_t>(value >> 16);
    data[2] = static_cast<uint8_t>(value >> 8);
    data[3] = static_cast<uint8_t>(value);
}

}  // namespace

extern "C" bool knx_dpt1_decode(const uint8_t *data, size_t length, bool *value)
{
    if (data == nullptr || value == nullptr || length < 1) {
        return false;
    }
    *value = (data[0] & 0x01U) != 0;
    return true;
}

extern "C" bool knx_dpt1_encode(bool value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 1) {
        return false;
    }
    data[0] = value ? 1U : 0U;
    return true;
}

extern "C" bool knx_dpt9_decode(const uint8_t *data, size_t length, float *value)
{
    if (data == nullptr || value == nullptr || length < 2) {
        return false;
    }

    const uint8_t exponent = (data[0] >> 3) & 0x0fU;
    int16_t mantissa = (int16_t)(((uint16_t)(data[0] & 0x07U) << 8) | data[1]);
    if ((data[0] & 0x80U) != 0) {
        mantissa = (int16_t)(mantissa | (int16_t)0xf800);
    }
    *value = 0.01f * (float)mantissa * (float)(1UL << exponent);
    return true;
}

extern "C" bool knx_dpt9_encode(float value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 2 || !std::isfinite(value) ||
        value < KNX_DPT9_ENCODER_MIN_VALUE || value > KNX_DPT9_MAX_VALUE) {
        return false;
    }

    const bool negative = value < 0.0f;
    const float magnitude = negative ? -value : value;
    int32_t mantissa = (int32_t)std::round(magnitude / 0.01f);
    uint8_t exponent = 0;

    while (mantissa > 2047 && exponent < 15) {
        mantissa >>= 1;
        ++exponent;
    }
    if (mantissa > 2047) {
        return false;
    }

    if (negative) {
        mantissa = (~mantissa + 1) & 0x07ff;
    }
    data[0] = (uint8_t)((negative ? 0x80U : 0U) | (exponent << 3) |
                        (((uint32_t)mantissa >> 8) & 0x07U));
    data[1] = (uint8_t)((uint32_t)mantissa & 0xffU);
    return true;
}

extern "C" bool knx_dpt5_decode(const uint8_t *data, size_t length, uint8_t *value)
{
    if (data == nullptr || value == nullptr || length < 1) return false;
    *value = data[0];
    return true;
}

extern "C" bool knx_dpt5_encode(uint8_t value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 1) return false;
    data[0] = value;
    return true;
}

extern "C" bool knx_dpt6_decode(const uint8_t *data, size_t length, int8_t *value)
{
    if (data == nullptr || value == nullptr || length < 1) return false;
    *value = static_cast<int8_t>(data[0]);
    return true;
}

extern "C" bool knx_dpt6_encode(int8_t value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 1) return false;
    data[0] = static_cast<uint8_t>(value);
    return true;
}

extern "C" bool knx_dpt7_decode(const uint8_t *data, size_t length, uint16_t *value)
{
    if (data == nullptr || value == nullptr || length < 2) return false;
    *value = read_be16(data);
    return true;
}

extern "C" bool knx_dpt7_encode(uint16_t value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 2) return false;
    write_be16(value, data);
    return true;
}

extern "C" bool knx_dpt8_decode(const uint8_t *data, size_t length, int16_t *value)
{
    if (data == nullptr || value == nullptr || length < 2) return false;
    *value = static_cast<int16_t>(read_be16(data));
    return true;
}

extern "C" bool knx_dpt8_encode(int16_t value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 2) return false;
    write_be16(static_cast<uint16_t>(value), data);
    return true;
}

extern "C" bool knx_dpt10_decode(const uint8_t *data, size_t length,
                                  knx_dpt10_time_t *value)
{
    if (data == nullptr || value == nullptr || length < 3) return false;
    value->weekday = static_cast<uint8_t>((data[0] >> 5) & 0x07U);
    value->hour = static_cast<uint8_t>(data[0] & 0x1fU);
    value->minute = static_cast<uint8_t>(data[1] & 0x3fU);
    value->second = static_cast<uint8_t>(data[2] & 0x3fU);
    return value->hour <= 23 && value->minute <= 59 && value->second <= 59;
}

extern "C" bool knx_dpt10_encode(const knx_dpt10_time_t *value, uint8_t *data,
                                  size_t length)
{
    if (value == nullptr || data == nullptr || length < 3 || value->weekday > 7 ||
        value->hour > 23 || value->minute > 59 || value->second > 59) return false;
    data[0] = static_cast<uint8_t>((value->weekday << 5) | value->hour);
    data[1] = value->minute;
    data[2] = value->second;
    return true;
}

extern "C" bool knx_dpt11_decode(const uint8_t *data, size_t length,
                                  knx_dpt11_date_t *value)
{
    if (data == nullptr || value == nullptr || length < 3) return false;
    value->day = static_cast<uint8_t>(data[0] & 0x1fU);
    value->month = static_cast<uint8_t>(data[1] & 0x0fU);
    value->year = static_cast<uint8_t>(data[2] & 0x7fU);
    return value->day >= 1 && value->day <= 31 &&
           value->month >= 1 && value->month <= 12 && value->year <= 99;
}

extern "C" bool knx_dpt11_encode(const knx_dpt11_date_t *value, uint8_t *data,
                                  size_t length)
{
    if (value == nullptr || data == nullptr || length < 3 || value->day < 1 ||
        value->day > 31 || value->month < 1 || value->month > 12 ||
        value->year > 99) return false;
    data[0] = value->day;
    data[1] = value->month;
    data[2] = value->year;
    return true;
}

extern "C" bool knx_dpt12_decode(const uint8_t *data, size_t length, uint32_t *value)
{
    if (data == nullptr || value == nullptr || length < 4) return false;
    *value = read_be32(data);
    return true;
}

extern "C" bool knx_dpt12_encode(uint32_t value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 4) return false;
    write_be32(value, data);
    return true;
}

extern "C" bool knx_dpt13_decode(const uint8_t *data, size_t length, int32_t *value)
{
    if (data == nullptr || value == nullptr || length < 4) return false;
    *value = static_cast<int32_t>(read_be32(data));
    return true;
}

extern "C" bool knx_dpt13_encode(int32_t value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 4) return false;
    write_be32(static_cast<uint32_t>(value), data);
    return true;
}

extern "C" bool knx_dpt14_decode(const uint8_t *data, size_t length, float *value)
{
    if (data == nullptr || value == nullptr || length < 4) return false;
    const uint32_t bits = read_be32(data);
    std::memcpy(value, &bits, sizeof(bits));
    return std::isfinite(*value);
}

extern "C" bool knx_dpt14_encode(float value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 4 || !std::isfinite(value)) return false;
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    write_be32(bits, data);
    return true;
}

extern "C" bool knx_dpt16_decode(const uint8_t *data, size_t length, char *value,
                                  size_t value_capacity)
{
    if (data == nullptr || value == nullptr || length < 14 || value_capacity < 15) return false;
    std::memcpy(value, data, 14);
    value[14] = '\0';
    return true;
}

extern "C" bool knx_dpt16_encode(const char *value, uint8_t *data, size_t length)
{
    if (value == nullptr || data == nullptr || length < 14) return false;
    const size_t value_length = std::strlen(value);
    if (value_length > 14) return false;
    std::memset(data, 0, 14);
    std::memcpy(data, value, value_length);
    return true;
}

extern "C" bool knx_dpt232_decode(const uint8_t *data, size_t length,
                                   knx_dpt232_color_t *value)
{
    if (data == nullptr || value == nullptr || length < 3) return false;
    value->red = data[0];
    value->green = data[1];
    value->blue = data[2];
    return true;
}

extern "C" bool knx_dpt232_encode(const knx_dpt232_color_t *value,
                                   uint8_t *data, size_t length)
{
    if (value == nullptr || data == nullptr || length < 3) return false;
    data[0] = value->red;
    data[1] = value->green;
    data[2] = value->blue;
    return true;
}