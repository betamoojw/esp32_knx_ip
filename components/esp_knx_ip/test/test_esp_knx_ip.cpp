#include "esp_knx_ip/knx_dpt.h"
#include "esp_knx_ip/knx_types.h"
#include "esp_knx_ip_internal/config.hpp"
#include "esp_knx_ip_internal/protocol.hpp"

#include "nvs_flash.h"
#include "unity.h"

#include <cstring>

namespace esp_knx_ip {
using internal::ParseResult;
using internal::build_routing_indication;
using internal::parse_routing_indication;
}  // namespace esp_knx_ip

TEST_CASE("DPT9 preserves fork encoder vectors", "[esp_knx_ip][dpt]")
{
    uint8_t encoded[2] = {};
    TEST_ASSERT_TRUE(knx_dpt9_encode(20.48f, encoded, sizeof(encoded)));
    TEST_ASSERT_EQUAL_HEX8(0x0c, encoded[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, encoded[1]);
    TEST_ASSERT_TRUE(knx_dpt9_encode(-20.48f, encoded, sizeof(encoded)));
    TEST_ASSERT_EQUAL_HEX8(0x8c, encoded[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, encoded[1]);
    TEST_ASSERT_FALSE(knx_dpt9_encode(KNX_DPT9_MIN_VALUE, encoded,
                                      sizeof(encoded)));

    const uint8_t standard_minimum[] = {0xf8, 0x00};
    float value = 0.0f;
    TEST_ASSERT_TRUE(knx_dpt9_decode(standard_minimum,
                                    sizeof(standard_minimum), &value));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, KNX_DPT9_MIN_VALUE, value);
}

TEST_CASE("extended DPT codecs preserve wire vectors", "[esp_knx_ip][dpt]")
{
    uint8_t data[8] = {};

    TEST_ASSERT_TRUE(knx_dpt4_ascii_encode('A', data, sizeof(data)));
    TEST_ASSERT_EQUAL_HEX8(0x41, data[0]);
    data[0] = 0x80;
    char ascii = 0;
    TEST_ASSERT_FALSE(knx_dpt4_ascii_decode(data, 1, &ascii));

    TEST_ASSERT_TRUE(knx_dpt5_angle_encode(180.0f, data, sizeof(data)));
    TEST_ASSERT_EQUAL_HEX8(0x80, data[0]);

    const knx_dpt26_scene_info_t scene = {true, 12};
    TEST_ASSERT_TRUE(knx_dpt26_encode(&scene, data, sizeof(data)));
    TEST_ASSERT_EQUAL_HEX8(0x4c, data[0]);
    knx_dpt26_scene_info_t decoded_scene = {};
    TEST_ASSERT_TRUE(knx_dpt26_decode(data, 1, &decoded_scene));
    TEST_ASSERT_TRUE(decoded_scene.active);
    TEST_ASSERT_EQUAL_UINT8(12, decoded_scene.scene_number);
    data[0] = 0x80;
    TEST_ASSERT_FALSE(knx_dpt26_decode(data, 1, &decoded_scene));

    const uint8_t invalid_utf8[] = {0xc0, 0x80, 0x00};
    char text[8] = {};
    TEST_ASSERT_FALSE(knx_dpt28_decode(invalid_utf8, sizeof(invalid_utf8),
                                       text, sizeof(text)));

    const uint8_t invalid_rgbw[] = {1, 2, 3, 4, 1, 0x0f};
    knx_dpt251_color_t rgbw = {};
    TEST_ASSERT_FALSE(knx_dpt251_decode(invalid_rgbw, sizeof(invalid_rgbw),
                                        &rgbw));
}

TEST_CASE("routing indication round trips and rejects truncation",
          "[esp_knx_ip][protocol]")
{
    const uint8_t data[] = {0x00, 0x12, 0x34};
    uint8_t packet[32] = {};
    size_t packet_length = 0;
    TEST_ASSERT_TRUE(esp_knx_ip::build_routing_indication(
        0x1101, knx_group_address(1, 2, 3), KNX_COMMAND_WRITE, data,
        sizeof(data), packet, sizeof(packet), &packet_length));

    knx_telegram_t telegram = {};
    TEST_ASSERT_EQUAL(static_cast<int>(esp_knx_ip::ParseResult::kOk),
                      static_cast<int>(esp_knx_ip::parse_routing_indication(
                          packet, packet_length, &telegram)));
    TEST_ASSERT_EQUAL(KNX_COMMAND_WRITE, telegram.command);
    TEST_ASSERT_EQUAL_HEX16(0x0a03, telegram.destination);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, telegram.data, sizeof(data));
    TEST_ASSERT_EQUAL(static_cast<int>(esp_knx_ip::ParseResult::kTruncated),
                      static_cast<int>(esp_knx_ip::parse_routing_indication(
                          packet, 5, &telegram)));
}

TEST_CASE("physical address persists in versioned NVS", "[esp_knx_ip][nvs]")
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        TEST_ESP_OK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    TEST_ESP_OK(result);
    TEST_ESP_OK(esp_knx_ip::internal::save_physical_address(0x1234));
    knx_address_t restored = 0;
    TEST_ESP_OK(esp_knx_ip::internal::load_physical_address(&restored));
    TEST_ASSERT_EQUAL_HEX16(0x1234, restored);

    TEST_ESP_OK(esp_knx_ip::internal::save_group_address("temperature", 0x0a03));
    restored = 0;
    TEST_ESP_OK(esp_knx_ip::internal::load_group_address("temperature", &restored));
    TEST_ASSERT_EQUAL_HEX16(0x0a03, restored);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                      esp_knx_ip::internal::save_group_address("name_is_too_long", 1));
}

TEST_CASE("routing indication parses independent command vectors",
          "[esp_knx_ip][protocol]")
{
    const uint8_t packets[][17] = {
        {0x06, 0x10, 0x05, 0x30, 0x00, 0x11, 0x29, 0x00, 0xbc,
         0xe0, 0x11, 0x01, 0x0a, 0x03, 0x01, 0x00, 0x00},
        {0x06, 0x10, 0x05, 0x30, 0x00, 0x11, 0x29, 0x00, 0xbc,
         0xe0, 0x11, 0x01, 0x0a, 0x03, 0x01, 0x00, 0x40},
        {0x06, 0x10, 0x05, 0x30, 0x00, 0x11, 0x29, 0x00, 0xbc,
         0xe0, 0x11, 0x01, 0x0a, 0x03, 0x01, 0x00, 0x81},
    };
    const knx_command_t commands[] = {
        KNX_COMMAND_READ, KNX_COMMAND_RESPONSE, KNX_COMMAND_WRITE,
    };
    for (size_t index = 0; index < 3; ++index) {
        knx_telegram_t telegram = {};
        TEST_ASSERT_EQUAL(
            static_cast<int>(esp_knx_ip::ParseResult::kOk),
            static_cast<int>(esp_knx_ip::parse_routing_indication(
                packets[index], sizeof(packets[index]), &telegram)));
        TEST_ASSERT_EQUAL(commands[index], telegram.command);
    }
}