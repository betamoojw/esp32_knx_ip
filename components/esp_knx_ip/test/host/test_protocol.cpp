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
    uint8_t data[14] = {};
    uint16_t unsigned_16 = 0;
    int32_t signed_32 = 0;
    float float_32 = 0.0f;
    char text[15] = {};

    assert(knx_dpt7_encode(0xabcdU, data, sizeof(data)));
    assert(data[0] == 0xab && data[1] == 0xcd);
    assert(knx_dpt7_decode(data, 2, &unsigned_16) && unsigned_16 == 0xabcdU);
    assert(knx_dpt13_encode(-1234567, data, sizeof(data)));
    assert(knx_dpt13_decode(data, 4, &signed_32) && signed_32 == -1234567);
    assert(knx_dpt14_encode(-12.5f, data, sizeof(data)));
    assert(knx_dpt14_decode(data, 4, &float_32) && float_32 == -12.5f);
    assert(knx_dpt16_encode("KNX native", data, sizeof(data)));
    assert(knx_dpt16_decode(data, sizeof(data), text, sizeof(text)));
    assert(std::strcmp(text, "KNX native") == 0);

    const knx_dpt10_time_t invalid_time = {0, 24, 0, 0};
    assert(!knx_dpt10_encode(&invalid_time, data, 3));
    const knx_dpt11_date_t invalid_date = {0, 1, 24};
    assert(!knx_dpt11_encode(&invalid_date, data, 3));
    assert(!knx_dpt16_encode("123456789012345", data, sizeof(data)));
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

}  // namespace

int main()
{
    test_dpt9_regression();
    test_other_dpt_round_trips();
    test_addresses();
    test_frame_round_trip();
    test_malformed_frames();
    test_independent_command_vectors();
    return 0;
}