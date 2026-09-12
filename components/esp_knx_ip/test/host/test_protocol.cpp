#include "esp_knx_ip/knx_dpt.h"
#include "esp_knx_ip/knx_types.h"
#include "esp_knx_ip_internal/protocol.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace esp_knx_ip {
using internal::ParseResult;
using internal::build_routing_indication;
using internal::parse_routing_indication;
}  // namespace esp_knx_ip

namespace {

void original_encode(float value, uint8_t output[2])
{
    const int sign = value < 0.0f;
    if (sign) {
        value = -value;
    }
    int exponent = 0;
    int mantissa = (int)std::round(value / 0.01f);
    while (mantissa > 2047) {
        mantissa >>= 1;
        ++exponent;
    }
    if (sign) {
        mantissa = (~mantissa + 1) & 0x07ff;
    }
    output[0] = (uint8_t)((sign << 7) | ((exponent & 0x0f) << 3) |
                          ((mantissa >> 8) & 0x07));
    output[1] = (uint8_t)(mantissa & 0xff);
}

void test_dpt9_regression()
{
    const float values[] = {0.0f, 0.01f, 1.0f, 20.47f, 20.48f, 21.5f,
                            123.45f, 670760.0f, -0.01f, -1.0f, -20.48f,
                            -273.15f, -670000.0f};
    for (float value : values) {
        uint8_t expected[2];
        uint8_t actual[2];
        original_encode(value, expected);
        assert(knx_dpt9_encode(value, actual, sizeof(actual)));
        assert(std::memcmp(expected, actual, sizeof(actual)) == 0);

        float decoded = 0.0f;
        assert(knx_dpt9_decode(actual, sizeof(actual), &decoded));
        const float tolerance = 0.01f * (float)(1U << ((actual[0] >> 3) & 0x0fU));
        assert(std::fabs(decoded - value) <= tolerance);
    }

    uint8_t output[2];
    assert(!knx_dpt9_encode(std::numeric_limits<float>::infinity(), output, 2));
    assert(!knx_dpt9_encode(std::numeric_limits<float>::quiet_NaN(), output, 2));
    assert(!knx_dpt9_encode(KNX_DPT9_MAX_VALUE + 1.0f, output, 2));
    assert(!knx_dpt9_encode(KNX_DPT9_MIN_VALUE, output, 2));
    assert(!knx_dpt9_encode(-671000.0f, output, 2));
    float short_decode = 0.0f;
    assert(!knx_dpt9_decode(output, 1, &short_decode));

    struct Vector {
        float value;
        uint8_t encoded[2];
    };
    const Vector vectors[] = {
        {0.0f, {0x00, 0x00}},
        {0.01f, {0x00, 0x01}},
        {20.47f, {0x07, 0xff}},
        {20.48f, {0x0c, 0x00}},
        {-0.01f, {0x87, 0xff}},
        {-20.48f, {0x8c, 0x00}},
        {KNX_DPT9_MAX_VALUE, {0x7f, 0xff}},
        {KNX_DPT9_ENCODER_MIN_VALUE, {0xf8, 0x01}},
    };
    for (const Vector &vector : vectors) {
        assert(knx_dpt9_encode(vector.value, output, sizeof(output)));
        assert(std::memcmp(output, vector.encoded, sizeof(output)) == 0);
    }

    const uint8_t standard_minimum[] = {0xf8, 0x00};
    float decoded = 0.0f;
    assert(knx_dpt9_decode(standard_minimum, sizeof(standard_minimum), &decoded));
    assert(decoded == KNX_DPT9_MIN_VALUE);
}

void test_other_dpt_round_trips()
{
    uint8_t data[32] = {};
    bool boolean_value = false;
    uint8_t unsigned_8 = 0;
    int8_t signed_8 = 0;
    uint16_t unsigned_16 = 0;
    int16_t signed_16 = 0;
    uint32_t unsigned_32 = 0;
    int32_t signed_32 = 0;
    int64_t signed_64 = 0;
    float float_32 = 0.0f;
    char text[32] = {};

    assert(knx_dpt1_encode(true, data, sizeof(data)));
    assert(knx_dpt1_decode(data, 1, &boolean_value) && boolean_value);
    data[0] = 0x02;
    assert(!knx_dpt1_decode(data, 1, &boolean_value));

    const knx_dpt2_control_t control_2 = {true, false};
    knx_dpt2_control_t decoded_control_2 = {};
    assert(knx_dpt2_encode(&control_2, data, sizeof(data)) && data[0] == 2);
    assert(knx_dpt2_decode(data, 1, &decoded_control_2));
    assert(decoded_control_2.control && !decoded_control_2.value);
    data[0] = 0x04;
    assert(!knx_dpt2_decode(data, 1, &decoded_control_2));

    const knx_dpt3_control_t control_3 = {true, 7};
    knx_dpt3_control_t decoded_control_3 = {};
    assert(knx_dpt3_encode(&control_3, data, sizeof(data)) && data[0] == 15);
    assert(knx_dpt3_decode(data, 1, &decoded_control_3));
    assert(decoded_control_3.control && decoded_control_3.step_code == 7);
    const knx_dpt3_control_t invalid_control_3 = {false, 8};
    assert(!knx_dpt3_encode(&invalid_control_3, data, sizeof(data)));
    data[0] = 0x10;
    assert(!knx_dpt3_decode(data, 1, &decoded_control_3));

    assert(knx_dpt4_encode('A', data, sizeof(data)));
    assert(knx_dpt4_decode(data, 1, &unsigned_8) && unsigned_8 == 'A');
    char ascii = 0;
    assert(knx_dpt4_ascii_encode('Z', data, sizeof(data)));
    assert(knx_dpt4_ascii_decode(data, 1, &ascii) && ascii == 'Z');
    data[0] = 0x80;
    assert(!knx_dpt4_ascii_decode(data, 1, &ascii));
    assert(knx_dpt5_encode(255, data, sizeof(data)));
    assert(knx_dpt5_decode(data, 1, &unsigned_8) && unsigned_8 == 255);
    assert(knx_dpt5_scaling_encode(50.0f, data, sizeof(data)) && data[0] == 128);
    assert(knx_dpt5_scaling_decode(data, 1, &float_32));
    assert(std::fabs(float_32 - 50.196f) < 0.001f);
    assert(!knx_dpt5_scaling_encode(-0.01f, data, sizeof(data)));
    assert(!knx_dpt5_scaling_encode(100.01f, data, sizeof(data)));
    assert(knx_dpt5_angle_encode(180.0f, data, sizeof(data)) && data[0] == 128);
    assert(knx_dpt5_angle_decode(data, 1, &float_32));
    assert(std::fabs(float_32 - 180.706f) < 0.001f);
    assert(!knx_dpt5_angle_encode(360.01f, data, sizeof(data)));

    assert(knx_dpt6_encode(-128, data, sizeof(data)));
    assert(knx_dpt6_decode(data, 1, &signed_8) && signed_8 == -128);

    assert(knx_dpt7_encode(0xabcdU, data, sizeof(data)));
    assert(data[0] == 0xab && data[1] == 0xcd);
    assert(knx_dpt7_decode(data, 2, &unsigned_16) && unsigned_16 == 0xabcdU);
    assert(knx_dpt8_encode(-32768, data, sizeof(data)));
    assert(knx_dpt8_decode(data, 2, &signed_16) && signed_16 == -32768);

    const knx_dpt10_time_t time = {7, 23, 59, 58};
    knx_dpt10_time_t decoded_time = {};
    assert(knx_dpt10_encode(&time, data, sizeof(data)));
    assert(knx_dpt10_decode(data, 3, &decoded_time));
    assert(decoded_time.weekday == 7 && decoded_time.hour == 23 &&
        decoded_time.minute == 59 && decoded_time.second == 58);
    data[1] = 0x40;
    assert(!knx_dpt10_decode(data, 3, &decoded_time));
    const knx_dpt11_date_t date = {29, 2, 24};
    knx_dpt11_date_t decoded_date = {};
    assert(knx_dpt11_encode(&date, data, sizeof(data)));
    assert(knx_dpt11_decode(data, 3, &decoded_date));
    assert(decoded_date.day == 29 && decoded_date.month == 2 &&
        decoded_date.year == 24);

    assert(knx_dpt12_encode(UINT32_MAX, data, sizeof(data)));
    assert(knx_dpt12_decode(data, 4, &unsigned_32) && unsigned_32 == UINT32_MAX);
    assert(knx_dpt13_encode(-1234567, data, sizeof(data)));
    assert(knx_dpt13_decode(data, 4, &signed_32) && signed_32 == -1234567);
    assert(knx_dpt14_encode(-12.5f, data, sizeof(data)));
    assert(knx_dpt14_decode(data, 4, &float_32) && float_32 == -12.5f);
    assert(!knx_dpt14_encode(std::numeric_limits<float>::infinity(), data, 4));
    assert(knx_dpt15_encode(0xfedcba98U, data, sizeof(data)));
    assert(knx_dpt15_decode(data, 4, &unsigned_32) && unsigned_32 == 0xfedcba98U);
    assert(knx_dpt16_encode("KNX native", data, sizeof(data)));
    assert(knx_dpt16_decode(data, 14, text, sizeof(text)));
    assert(std::strcmp(text, "KNX native") == 0);

    assert(knx_dpt17_encode(63, data, sizeof(data)));
    assert(knx_dpt17_decode(data, 1, &unsigned_8) && unsigned_8 == 63);
    assert(!knx_dpt17_encode(64, data, sizeof(data)));
    const knx_dpt18_scene_control_t scene_control = {true, 42};
    knx_dpt18_scene_control_t decoded_scene_control = {};
    assert(knx_dpt18_encode(&scene_control, data, sizeof(data)));
    assert(knx_dpt18_decode(data, 1, &decoded_scene_control));
    assert(decoded_scene_control.learn && decoded_scene_control.scene_number == 42);

    const knx_dpt19_datetime_t date_time = {
     124, 2, 29, 4, 12, 34, 56, false, true, true, true, true, true, true, true,
    };
    knx_dpt19_datetime_t decoded_date_time = {};
    assert(knx_dpt19_encode(&date_time, data, sizeof(data)));
    assert(knx_dpt19_decode(data, 8, &decoded_date_time));
    assert(decoded_date_time.year == 124 && decoded_date_time.month == 2 &&
        decoded_date_time.day == 29 && decoded_date_time.time_valid &&
        decoded_date_time.daylight_saving_time && decoded_date_time.clock_quality);

    assert(knx_dpt20_encode(20, data, sizeof(data)));
    assert(knx_dpt20_decode(data, 1, &unsigned_8) && unsigned_8 == 20);
    assert(knx_dpt21_encode(0xa5, data, sizeof(data)));
    assert(knx_dpt21_decode(data, 1, &unsigned_8) && unsigned_8 == 0xa5);
    assert(knx_dpt22_encode(0xa55a, data, sizeof(data)));
    assert(knx_dpt22_decode(data, 2, &unsigned_16) && unsigned_16 == 0xa55a);
    assert(knx_dpt23_encode(3, data, sizeof(data)));
    assert(knx_dpt23_decode(data, 1, &unsigned_8) && unsigned_8 == 3);
    assert(!knx_dpt23_encode(4, data, sizeof(data)));

    size_t encoded_length = 0;
    assert(knx_dpt24_encode("KNX", data, sizeof(data), &encoded_length));
    assert(encoded_length == 4 && knx_dpt24_decode(data, encoded_length, text,
                              sizeof(text)));
    assert(std::strcmp(text, "KNX") == 0);
    assert(knx_dpt25_encode(0xff, data, sizeof(data)));
    assert(knx_dpt25_decode(data, 1, &unsigned_8) && unsigned_8 == 0xff);
    const knx_dpt26_scene_info_t scene_info = {true, 12};
    knx_dpt26_scene_info_t decoded_scene_info = {};
    assert(knx_dpt26_encode(&scene_info, data, sizeof(data)) && data[0] == 0x4c);
    assert(knx_dpt26_decode(data, 1, &decoded_scene_info));
    assert(decoded_scene_info.active && decoded_scene_info.scene_number == 12);
    data[0] = 0x80;
    assert(!knx_dpt26_decode(data, 1, &decoded_scene_info));

    const knx_dpt27_combined_status_t status = {0xa55a, 0x0ff0};
    knx_dpt27_combined_status_t decoded_status = {};
    assert(knx_dpt27_encode(&status, data, sizeof(data)));
    assert(knx_dpt27_decode(data, 4, &decoded_status));
    assert(decoded_status.value == status.value && decoded_status.mask == status.mask);

    const char utf8[] = "Gr\xc3\xbc\xc3\x9f";
    assert(knx_dpt28_encode(utf8, data, sizeof(data), &encoded_length));
    assert(knx_dpt28_decode(data, encoded_length, text, sizeof(text)));
    assert(std::strcmp(text, utf8) == 0);
    const char invalid_utf8[] = {static_cast<char>(0xc0), static_cast<char>(0x80), 0};
    assert(!knx_dpt28_encode(invalid_utf8, data, sizeof(data), &encoded_length));

    assert(knx_dpt29_encode(INT64_MIN, data, sizeof(data)));
    assert(knx_dpt29_decode(data, 8, &signed_64) && signed_64 == INT64_MIN);
    assert(knx_dpt30_encode(0xa5, data, sizeof(data)));
    assert(knx_dpt30_decode(data, 1, &unsigned_8) && unsigned_8 == 0xa5);
    assert(knx_dpt31_encode(0xabcdef, data, sizeof(data)));
    assert(knx_dpt31_decode(data, 3, &unsigned_32) && unsigned_32 == 0xabcdef);
    assert(!knx_dpt31_encode(0x01000000, data, sizeof(data)));

    const knx_dpt232_color_t color = {1, 2, 3};
    knx_dpt232_color_t decoded_color = {};
    assert(knx_dpt232_encode(&color, data, sizeof(data)));
    assert(knx_dpt232_decode(data, 3, &decoded_color));
    assert(decoded_color.red == 1 && decoded_color.green == 2 &&
        decoded_color.blue == 3);
    char language[3] = {};
    assert(knx_dpt234_encode("de", data, sizeof(data)));
    assert(knx_dpt234_decode(data, 2, language) && std::strcmp(language, "de") == 0);
    assert(!knx_dpt234_encode("DE", data, sizeof(data)));

    const knx_dpt251_color_t color_w = {1, 2, 3, 4, 0x0f};
    knx_dpt251_color_t decoded_color_w = {};
    assert(knx_dpt251_encode(&color_w, data, sizeof(data)));
    assert(knx_dpt251_decode(data, 6, &decoded_color_w));
    assert(decoded_color_w.white == 4 && decoded_color_w.valid_channels == 0x0f);

    const knx_dpt10_time_t invalid_time = {0, 24, 0, 0};
    assert(!knx_dpt10_encode(&invalid_time, data, 3));
    const knx_dpt11_date_t invalid_date_value = {29, 2, 23};
    assert(!knx_dpt11_encode(&invalid_date_value, data, 3));
    assert(!knx_dpt16_encode("123456789012345", data, sizeof(data)));
    assert(!knx_dpt1_decode(nullptr, 1, &boolean_value));
    assert(!knx_dpt29_decode(data, 7, &signed_64));
}

void test_addresses()
{
    const knx_address_t group = knx_group_address(31, 7, 255);
    assert(group == 0xffffU);
    assert(knx_group_main(group) == 31);
    assert(knx_group_middle(group) == 7);
    assert(knx_group_sub(group) == 255);

    const knx_address_t physical = knx_physical_address(1, 2, 3);
    assert(physical == 0x1203U);
    assert(knx_physical_area(physical) == 1);
    assert(knx_physical_line(physical) == 2);
    assert(knx_physical_member(physical) == 3);
}

void test_frame_round_trip()
{
    const uint8_t apdu[] = {0x00, 0x12, 0x34};
    uint8_t frame[64] = {};
    size_t frame_length = 0;
    assert(esp_knx_ip::build_routing_indication(0x1101, 0x0903,
                                                KNX_COMMAND_WRITE, apdu,
                                                sizeof(apdu), frame,
                                                sizeof(frame), &frame_length));
    const uint8_t expected[] = {
        0x06, 0x10, 0x05, 0x30, 0x00, 0x13, 0x29, 0x00, 0xbc, 0xe0,
        0x11, 0x01, 0x09, 0x03, 0x03, 0x00, 0x80, 0x12, 0x34,
    };
    assert(frame_length == sizeof(expected));
    assert(std::memcmp(frame, expected, sizeof(expected)) == 0);

    knx_telegram_t telegram = {};
    assert(esp_knx_ip::parse_routing_indication(frame, frame_length, &telegram) ==
           esp_knx_ip::ParseResult::kOk);
    assert(telegram.command == KNX_COMMAND_WRITE);
    assert(telegram.source == 0x1101);
    assert(telegram.destination == 0x0903);
    assert(telegram.data_length == sizeof(apdu));
    assert(std::memcmp(telegram.data, apdu, sizeof(apdu)) == 0);

    frame[4] = 0xff;
    assert(esp_knx_ip::parse_routing_indication(frame, frame_length, &telegram) ==
           esp_knx_ip::ParseResult::kInvalidLength);
    assert(esp_knx_ip::parse_routing_indication(frame, 5, &telegram) ==
           esp_knx_ip::ParseResult::kTruncated);

    uint8_t zero_group_frame[32] = {};
    assert(esp_knx_ip::build_routing_indication(
        0x1101, 0, KNX_COMMAND_READ, apdu, sizeof(apdu), zero_group_frame,
        sizeof(zero_group_frame), &frame_length));
}

void test_malformed_frames()
{
    const uint8_t apdu[] = {0};
    uint8_t frame[32] = {};
    size_t frame_length = 0;
    assert(esp_knx_ip::build_routing_indication(
        0x1101, 0x0101, KNX_COMMAND_READ, apdu, sizeof(apdu), frame,
        sizeof(frame), &frame_length));
    knx_telegram_t telegram = {};

    uint8_t changed[32];
    std::memcpy(changed, frame, frame_length);
    changed[0] = 5;
    assert(esp_knx_ip::parse_routing_indication(changed, frame_length, &telegram) ==
           esp_knx_ip::ParseResult::kInvalidHeader);
    std::memcpy(changed, frame, frame_length);
    changed[3] = 0x20;
    assert(esp_knx_ip::parse_routing_indication(changed, frame_length, &telegram) ==
           esp_knx_ip::ParseResult::kUnsupportedService);
    std::memcpy(changed, frame, frame_length);
    changed[6] = 0x11;
    assert(esp_knx_ip::parse_routing_indication(changed, frame_length, &telegram) ==
           esp_knx_ip::ParseResult::kUnsupportedMessage);
    std::memcpy(changed, frame, frame_length);
    changed[9] &= 0x7fU;
    assert(esp_knx_ip::parse_routing_indication(changed, frame_length, &telegram) ==
           esp_knx_ip::ParseResult::kNotGroupAddressed);
    std::memcpy(changed, frame, frame_length);
    changed[7] = 20;
    assert(esp_knx_ip::parse_routing_indication(changed, frame_length, &telegram) ==
           esp_knx_ip::ParseResult::kTruncated);
}

void test_independent_command_vectors()
{
    const uint8_t read_frame[] = {
        0x06, 0x10, 0x05, 0x30, 0x00, 0x11, 0x29, 0x00, 0xbc,
        0xe0, 0x11, 0x01, 0x0a, 0x03, 0x01, 0x00, 0x00,
    };
    const uint8_t response_frame[] = {
        0x06, 0x10, 0x05, 0x30, 0x00, 0x11, 0x29, 0x00, 0xbc,
        0xe0, 0x11, 0x01, 0x0a, 0x03, 0x01, 0x00, 0x40,
    };
    const uint8_t write_frame[] = {
        0x06, 0x10, 0x05, 0x30, 0x00, 0x11, 0x29, 0x00, 0xbc,
        0xe0, 0x11, 0x01, 0x0a, 0x03, 0x01, 0x00, 0x81,
    };
    struct Vector {
        const uint8_t *packet;
        knx_command_t command;
        uint8_t value;
    };
    const Vector vectors[] = {
        {read_frame, KNX_COMMAND_READ, 0},
        {response_frame, KNX_COMMAND_RESPONSE, 0},
        {write_frame, KNX_COMMAND_WRITE, 1},
    };
    for (const Vector &vector : vectors) {
        knx_telegram_t telegram = {};
        assert(esp_knx_ip::parse_routing_indication(
                   vector.packet, sizeof(read_frame), &telegram) ==
               esp_knx_ip::ParseResult::kOk);
        assert(telegram.command == vector.command);
        assert(telegram.data_length == 1);
        assert(telegram.data[0] == vector.value);
    }
}

void test_dpt_telegram_integration()
{
    uint8_t encoded[2] = {};
    assert(knx_dpt9_encode(21.5f, encoded, sizeof(encoded)));
    const uint8_t apdu[] = {0, encoded[0], encoded[1]};
    uint8_t frame[32] = {};
    size_t frame_length = 0;
    assert(esp_knx_ip::build_routing_indication(
        0x1101, 0x0a03, KNX_COMMAND_RESPONSE, apdu, sizeof(apdu), frame,
        sizeof(frame), &frame_length));

    knx_telegram_t telegram = {};
    assert(esp_knx_ip::parse_routing_indication(frame, frame_length, &telegram) ==
           esp_knx_ip::ParseResult::kOk);
    assert(telegram.command == KNX_COMMAND_RESPONSE && telegram.data_length == 3);
    float decoded = 0.0f;
    assert(knx_dpt9_decode(telegram.data + 1, telegram.data_length - 1, &decoded));
    assert(std::fabs(decoded - 21.5f) <= 0.02f);
}

}  // namespace

int main()
{
    test_dpt9_regression();
    test_other_dpt_round_trips();
    test_addresses();
    test_frame_round_trip();
    test_malformed_frames();
    test_independent_command_vectors();
    test_dpt_telegram_integration();
    return 0;
}