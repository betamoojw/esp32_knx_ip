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

uint64_t read_be64(const uint8_t *data)
{
    uint64_t value = 0;
    for (size_t index = 0; index < 8; ++index) {
        value = (value << 8) | data[index];
    }
    return value;
}

void write_be64(uint64_t value, uint8_t *data)
{
    for (size_t index = 0; index < 8; ++index) {
        data[7 - index] = static_cast<uint8_t>(value);
        value >>= 8;
    }
}

bool valid_date(uint16_t year, uint8_t month, uint8_t day)
{
    if (month < 1 || month > 12 || day < 1) return false;
    static constexpr uint8_t days_per_month[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
    };
    uint8_t maximum = days_per_month[month - 1];
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    if (month == 2 && leap) ++maximum;
    return day <= maximum;
}

bool valid_utf8(const char *value, size_t length)
{
    size_t index = 0;
    while (index < length) {
        const uint8_t first = static_cast<uint8_t>(value[index++]);
        if (first <= 0x7fU) continue;
        size_t continuation_count = 0;
        uint32_t codepoint = 0;
        if (first >= 0xc2U && first <= 0xdfU) {
            continuation_count = 1;
            codepoint = first & 0x1fU;
        } else if (first >= 0xe0U && first <= 0xefU) {
            continuation_count = 2;
            codepoint = first & 0x0fU;
        } else if (first >= 0xf0U && first <= 0xf4U) {
            continuation_count = 3;
            codepoint = first & 0x07U;
        } else {
            return false;
        }
        if (index + continuation_count > length) return false;
        for (size_t count = 0; count < continuation_count; ++count) {
            const uint8_t next = static_cast<uint8_t>(value[index++]);
            if ((next & 0xc0U) != 0x80U) return false;
            codepoint = (codepoint << 6) | (next & 0x3fU);
        }
        if ((continuation_count == 2 && codepoint < 0x800U) ||
            (continuation_count == 3 && codepoint < 0x10000U) ||
            (codepoint >= 0xd800U && codepoint <= 0xdfffU) ||
            codepoint > 0x10ffffU) return false;
    }
    return true;
}

bool decode_terminated_text(const uint8_t *data, size_t length, char *value,
                            size_t value_capacity, bool require_utf8)
{
    if (data == nullptr || value == nullptr || length == 0) return false;
    const void *terminator = std::memchr(data, 0, length);
    if (terminator == nullptr) return false;
    const size_t text_length = static_cast<const uint8_t *>(terminator) - data;
    if (value_capacity <= text_length ||
        (require_utf8 && !valid_utf8(reinterpret_cast<const char *>(data),
                                     text_length))) return false;
    std::memcpy(value, data, text_length + 1);
    return true;
}

bool encode_terminated_text(const char *value, uint8_t *data, size_t length,
                            size_t *encoded_length, bool require_utf8)
{
    if (value == nullptr || data == nullptr || encoded_length == nullptr) return false;
    const size_t text_length = std::strlen(value);
    if (text_length + 1 > length ||
        (require_utf8 && !valid_utf8(value, text_length))) return false;
    std::memcpy(data, value, text_length + 1);
    *encoded_length = text_length + 1;
    return true;
}

}  // namespace

extern "C" bool knx_dpt1_decode(const uint8_t *data, size_t length, bool *value)
{
    if (data == nullptr || value == nullptr || length < 1 ||
        (data[0] & 0xfeU) != 0) {
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

extern "C" bool knx_dpt2_decode(const uint8_t *data, size_t length,
                                  knx_dpt2_control_t *value)
{
    if (data == nullptr || value == nullptr || length < 1 ||
        (data[0] & 0xfcU) != 0) return false;
    value->control = (data[0] & 0x02U) != 0;
    value->value = (data[0] & 0x01U) != 0;
    return true;
}

extern "C" bool knx_dpt2_encode(const knx_dpt2_control_t *value,
                                  uint8_t *data, size_t length)
{
    if (value == nullptr || data == nullptr || length < 1) return false;
    data[0] = static_cast<uint8_t>((value->control ? 0x02U : 0U) |
                                   (value->value ? 0x01U : 0U));
    return true;
}

extern "C" bool knx_dpt3_decode(const uint8_t *data, size_t length,
                                  knx_dpt3_control_t *value)
{
    if (data == nullptr || value == nullptr || length < 1 ||
        (data[0] & 0xf0U) != 0) return false;
    value->control = (data[0] & 0x08U) != 0;
    value->step_code = static_cast<uint8_t>(data[0] & 0x07U);
    return true;
}

extern "C" bool knx_dpt3_encode(const knx_dpt3_control_t *value,
                                  uint8_t *data, size_t length)
{
    if (value == nullptr || data == nullptr || length < 1 ||
        value->step_code > 7) return false;
    data[0] = static_cast<uint8_t>((value->control ? 0x08U : 0U) |
                                   value->step_code);
    return true;
}

extern "C" bool knx_dpt4_decode(const uint8_t *data, size_t length,
                                  uint8_t *value)
{
    if (data == nullptr || value == nullptr || length < 1) return false;
    *value = data[0];
    return true;
}

extern "C" bool knx_dpt4_encode(uint8_t value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 1) return false;
    data[0] = value;
    return true;
}

extern "C" bool knx_dpt4_ascii_decode(const uint8_t *data, size_t length,
                                         char *value)
{
    if (data == nullptr || value == nullptr || length < 1 || data[0] > 0x7fU) {
        return false;
    }
    *value = static_cast<char>(data[0]);
    return true;
}

extern "C" bool knx_dpt4_ascii_encode(char value, uint8_t *data, size_t length)
{
    const uint8_t encoded = static_cast<uint8_t>(value);
    if (data == nullptr || length < 1 || encoded > 0x7fU) return false;
    data[0] = encoded;
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

extern "C" bool knx_dpt5_scaling_decode(const uint8_t *data, size_t length,
                                          float *value)
{
    if (data == nullptr || value == nullptr || length < 1) return false;
    *value = static_cast<float>(data[0]) * 100.0f / 255.0f;
    return true;
}

extern "C" bool knx_dpt5_scaling_encode(float value, uint8_t *data,
                                          size_t length)
{
    if (data == nullptr || length < 1 || !std::isfinite(value) ||
        value < 0.0f || value > 100.0f) return false;
    data[0] = static_cast<uint8_t>(std::round(value * 255.0f / 100.0f));
    return true;
}

extern "C" bool knx_dpt5_angle_decode(const uint8_t *data, size_t length,
                                        float *value)
{
    if (data == nullptr || value == nullptr || length < 1) return false;
    *value = static_cast<float>(data[0]) * 360.0f / 255.0f;
    return true;
}

extern "C" bool knx_dpt5_angle_encode(float value, uint8_t *data,
                                        size_t length)
{
    if (data == nullptr || length < 1 || !std::isfinite(value) ||
        value < 0.0f || value > 360.0f) return false;
    data[0] = static_cast<uint8_t>(std::round(value * 255.0f / 360.0f));
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
    if (data == nullptr || value == nullptr || length < 3 ||
        (data[1] & 0xc0U) != 0 || (data[2] & 0xc0U) != 0) return false;
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
    if ((data[0] & 0xe0U) != 0 || (data[1] & 0xf0U) != 0 ||
        (data[2] & 0x80U) != 0) return false;
    value->day = static_cast<uint8_t>(data[0] & 0x1fU);
    value->month = static_cast<uint8_t>(data[1] & 0x0fU);
    value->year = static_cast<uint8_t>(data[2] & 0x7fU);
    const uint16_t full_year = value->year >= 90 ?
        static_cast<uint16_t>(1900 + value->year) :
        static_cast<uint16_t>(2000 + value->year);
    return value->year <= 99 && valid_date(full_year, value->month, value->day);
}

extern "C" bool knx_dpt11_encode(const knx_dpt11_date_t *value, uint8_t *data,
                                  size_t length)
{
    if (value == nullptr || data == nullptr || length < 3 || value->day < 1 ||
        value->year > 99) return false;
    const uint16_t full_year = value->year >= 90 ?
        static_cast<uint16_t>(1900 + value->year) :
        static_cast<uint16_t>(2000 + value->year);
    if (!valid_date(full_year, value->month, value->day)) return false;
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

extern "C" bool knx_dpt15_decode(const uint8_t *data, size_t length,
                                   uint32_t *value)
{
    return knx_dpt12_decode(data, length, value);
}

extern "C" bool knx_dpt15_encode(uint32_t value, uint8_t *data, size_t length)
{
    return knx_dpt12_encode(value, data, length);
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

extern "C" bool knx_dpt17_decode(const uint8_t *data, size_t length,
                                   uint8_t *scene_number)
{
    if (data == nullptr || scene_number == nullptr || length < 1 ||
        (data[0] & 0xc0U) != 0) return false;
    *scene_number = data[0];
    return true;
}

extern "C" bool knx_dpt17_encode(uint8_t scene_number, uint8_t *data,
                                   size_t length)
{
    if (data == nullptr || length < 1 || scene_number > 63) return false;
    data[0] = scene_number;
    return true;
}

extern "C" bool knx_dpt18_decode(const uint8_t *data, size_t length,
                                   knx_dpt18_scene_control_t *value)
{
    if (data == nullptr || value == nullptr || length < 1 ||
        (data[0] & 0x40U) != 0) return false;
    value->learn = (data[0] & 0x80U) != 0;
    value->scene_number = static_cast<uint8_t>(data[0] & 0x3fU);
    return true;
}

extern "C" bool knx_dpt18_encode(const knx_dpt18_scene_control_t *value,
                                   uint8_t *data, size_t length)
{
    if (value == nullptr || data == nullptr || length < 1 ||
        value->scene_number > 63) return false;
    data[0] = static_cast<uint8_t>((value->learn ? 0x80U : 0U) |
                                   value->scene_number);
    return true;
}

extern "C" bool knx_dpt19_decode(const uint8_t *data, size_t length,
                                   knx_dpt19_datetime_t *value)
{
    if (data == nullptr || value == nullptr || length < 8 ||
        (data[1] & 0xf0U) != 0 || (data[2] & 0xe0U) != 0 ||
        (data[4] & 0xc0U) != 0 || (data[5] & 0xc0U) != 0 ||
        (data[7] & 0x7fU) != 0) return false;
    value->year = data[0];
    value->month = static_cast<uint8_t>(data[1] & 0x0fU);
    value->day = static_cast<uint8_t>(data[2] & 0x1fU);
    value->weekday = static_cast<uint8_t>((data[3] >> 5) & 0x07U);
    value->hour = static_cast<uint8_t>(data[3] & 0x1fU);
    value->minute = static_cast<uint8_t>(data[4] & 0x3fU);
    value->second = static_cast<uint8_t>(data[5] & 0x3fU);
    value->fault = (data[6] & 0x80U) != 0;
    value->working_day = (data[6] & 0x40U) != 0;
    value->working_day_valid = (data[6] & 0x20U) == 0;
    value->date_valid = (data[6] & 0x18U) == 0;
    value->weekday_valid = (data[6] & 0x04U) == 0;
    value->time_valid = (data[6] & 0x02U) == 0;
    value->daylight_saving_time = (data[6] & 0x01U) != 0;
    value->clock_quality = (data[7] & 0x80U) != 0;
    if (value->weekday > 7) return false;
    if (value->date_valid &&
        !valid_date(static_cast<uint16_t>(1900 + value->year), value->month,
                    value->day)) return false;
    return !value->time_valid ||
           (value->hour <= 23 && value->minute <= 59 && value->second <= 59);
}

extern "C" bool knx_dpt19_encode(const knx_dpt19_datetime_t *value,
                                   uint8_t *data, size_t length)
{
    if (value == nullptr || data == nullptr || length < 8 ||
        value->month > 15 || value->day > 31 || value->weekday > 7 ||
        value->hour > 31 || value->minute > 63 || value->second > 63 ||
        (value->date_valid &&
         !valid_date(static_cast<uint16_t>(1900 + value->year), value->month,
                     value->day)) ||
        (value->time_valid &&
         (value->hour > 23 || value->minute > 59 || value->second > 59))) return false;
    data[0] = value->year;
    data[1] = static_cast<uint8_t>(value->month & 0x0fU);
    data[2] = static_cast<uint8_t>(value->day & 0x1fU);
    data[3] = static_cast<uint8_t>((value->weekday << 5) | (value->hour & 0x1fU));
    data[4] = static_cast<uint8_t>(value->minute & 0x3fU);
    data[5] = static_cast<uint8_t>(value->second & 0x3fU);
    data[6] = static_cast<uint8_t>((value->fault ? 0x80U : 0U) |
        (value->working_day ? 0x40U : 0U) |
        (!value->working_day_valid ? 0x20U : 0U) |
        (!value->date_valid ? 0x18U : 0U) |
        (!value->weekday_valid ? 0x04U : 0U) |
        (!value->time_valid ? 0x02U : 0U) |
        (value->daylight_saving_time ? 0x01U : 0U));
    data[7] = value->clock_quality ? 0x80U : 0U;
    return true;
}

extern "C" bool knx_dpt20_decode(const uint8_t *data, size_t length,
                                   uint8_t *value)
{
    return knx_dpt5_decode(data, length, value);
}

extern "C" bool knx_dpt20_encode(uint8_t value, uint8_t *data, size_t length)
{
    return knx_dpt5_encode(value, data, length);
}

extern "C" bool knx_dpt21_decode(const uint8_t *data, size_t length,
                                   uint8_t *value)
{
    return knx_dpt5_decode(data, length, value);
}

extern "C" bool knx_dpt21_encode(uint8_t value, uint8_t *data, size_t length)
{
    return knx_dpt5_encode(value, data, length);
}

extern "C" bool knx_dpt22_decode(const uint8_t *data, size_t length,
                                   uint16_t *value)
{
    return knx_dpt7_decode(data, length, value);
}

extern "C" bool knx_dpt22_encode(uint16_t value, uint8_t *data, size_t length)
{
    return knx_dpt7_encode(value, data, length);
}

extern "C" bool knx_dpt23_decode(const uint8_t *data, size_t length,
                                   uint8_t *value)
{
    if (data == nullptr || value == nullptr || length < 1 ||
        (data[0] & 0xfcU) != 0) return false;
    *value = data[0];
    return true;
}

extern "C" bool knx_dpt23_encode(uint8_t value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 1 || value > 3) return false;
    data[0] = value;
    return true;
}

extern "C" bool knx_dpt24_decode(const uint8_t *data, size_t length,
                                   char *value, size_t value_capacity)
{
    return decode_terminated_text(data, length, value, value_capacity, false);
}

extern "C" bool knx_dpt24_encode(const char *value, uint8_t *data,
                                   size_t length, size_t *encoded_length)
{
    return encode_terminated_text(value, data, length, encoded_length, false);
}

extern "C" bool knx_dpt25_decode(const uint8_t *data, size_t length,
                                   uint8_t *value)
{
    return knx_dpt5_decode(data, length, value);
}

extern "C" bool knx_dpt25_encode(uint8_t value, uint8_t *data, size_t length)
{
    return knx_dpt5_encode(value, data, length);
}

extern "C" bool knx_dpt26_decode(const uint8_t *data, size_t length,
                                   knx_dpt26_scene_info_t *value)
{
    if (data == nullptr || value == nullptr || length < 1 ||
        (data[0] & 0x80U) != 0) return false;
    value->active = (data[0] & 0x40U) != 0;
    value->scene_number = static_cast<uint8_t>(data[0] & 0x3fU);
    return true;
}

extern "C" bool knx_dpt26_encode(const knx_dpt26_scene_info_t *value,
                                   uint8_t *data, size_t length)
{
    if (value == nullptr || data == nullptr || length < 1 ||
        value->scene_number > 63) return false;
    data[0] = static_cast<uint8_t>((value->active ? 0x40U : 0U) |
                                   value->scene_number);
    return true;
}

extern "C" bool knx_dpt27_decode(const uint8_t *data, size_t length,
                                   knx_dpt27_combined_status_t *value)
{
    if (data == nullptr || value == nullptr || length < 4) return false;
    value->value = read_be16(data);
    value->mask = read_be16(data + 2);
    return true;
}

extern "C" bool knx_dpt27_encode(const knx_dpt27_combined_status_t *value,
                                   uint8_t *data, size_t length)
{
    if (value == nullptr || data == nullptr || length < 4) return false;
    write_be16(value->value, data);
    write_be16(value->mask, data + 2);
    return true;
}

extern "C" bool knx_dpt28_decode(const uint8_t *data, size_t length,
                                   char *value, size_t value_capacity)
{
    return decode_terminated_text(data, length, value, value_capacity, true);
}

extern "C" bool knx_dpt28_encode(const char *value, uint8_t *data,
                                   size_t length, size_t *encoded_length)
{
    return encode_terminated_text(value, data, length, encoded_length, true);
}

extern "C" bool knx_dpt29_decode(const uint8_t *data, size_t length,
                                   int64_t *value)
{
    if (data == nullptr || value == nullptr || length < 8) return false;
    const uint64_t bits = read_be64(data);
    std::memcpy(value, &bits, sizeof(bits));
    return true;
}

extern "C" bool knx_dpt29_encode(int64_t value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 8) return false;
    uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    write_be64(bits, data);
    return true;
}

extern "C" bool knx_dpt30_decode(const uint8_t *data, size_t length,
                                   uint8_t *value)
{
    return knx_dpt5_decode(data, length, value);
}

extern "C" bool knx_dpt30_encode(uint8_t value, uint8_t *data, size_t length)
{
    return knx_dpt5_encode(value, data, length);
}

extern "C" bool knx_dpt31_decode(const uint8_t *data, size_t length,
                                   uint32_t *value)
{
    if (data == nullptr || value == nullptr || length < 3) return false;
    *value = (static_cast<uint32_t>(data[0]) << 16) |
             (static_cast<uint32_t>(data[1]) << 8) | data[2];
    return true;
}

extern "C" bool knx_dpt31_encode(uint32_t value, uint8_t *data, size_t length)
{
    if (data == nullptr || length < 3 || value > 0x00ffffffU) return false;
    data[0] = static_cast<uint8_t>(value >> 16);
    data[1] = static_cast<uint8_t>(value >> 8);
    data[2] = static_cast<uint8_t>(value);
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

extern "C" bool knx_dpt234_decode(const uint8_t *data, size_t length,
                                    char language[3])
{
    if (data == nullptr || language == nullptr || length < 2 ||
        data[0] < 'a' || data[0] > 'z' || data[1] < 'a' || data[1] > 'z') return false;
    language[0] = static_cast<char>(data[0]);
    language[1] = static_cast<char>(data[1]);
    language[2] = '\0';
    return true;
}

extern "C" bool knx_dpt234_encode(const char language[3], uint8_t *data,
                                    size_t length)
{
    if (language == nullptr || data == nullptr || length < 2 ||
        language[0] < 'a' || language[0] > 'z' ||
        language[1] < 'a' || language[1] > 'z' || language[2] != '\0') return false;
    data[0] = static_cast<uint8_t>(language[0]);
    data[1] = static_cast<uint8_t>(language[1]);
    return true;
}

extern "C" bool knx_dpt251_decode(const uint8_t *data, size_t length,
                                    knx_dpt251_color_t *value)
{
    if (data == nullptr || value == nullptr || length < 6 || data[4] != 0 ||
        (data[5] & 0xf0U) != 0) return false;
    value->red = data[0];
    value->green = data[1];
    value->blue = data[2];
    value->white = data[3];
    value->valid_channels = data[5];
    return true;
}

extern "C" bool knx_dpt251_encode(const knx_dpt251_color_t *value,
                                    uint8_t *data, size_t length)
{
    if (value == nullptr || data == nullptr || length < 6 ||
        (value->valid_channels & 0xf0U) != 0) return false;
    data[0] = value->red;
    data[1] = value->green;
    data[2] = value->blue;
    data[3] = value->white;
    data[4] = 0;
    data[5] = value->valid_channels;
    return true;
}