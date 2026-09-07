#include "esp_knx_ip_web/esp_knx_ip_webserver.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>

namespace {

esp_err_t send_error(httpd_req_t *request, httpd_err_code_t status,
                     const char *message)
{
    httpd_resp_send_err(request, status, message);
    return ESP_FAIL;
}

esp_err_t get_config(httpd_req_t *request)
{
    auto handle = static_cast<esp_knx_ip_handle_t>(request->user_ctx);
    knx_address_t address = 0;
    esp_knx_ip_state_t state = ESP_KNX_IP_STATE_STOPPED;
    esp_err_t result = esp_knx_ip_get_physical_address(handle, &address);
    if (result == ESP_OK) {
        result = esp_knx_ip_get_state(handle, &state);
    }
    if (result != ESP_OK) {
        return send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Unable to read KNX configuration");
    }

    char response[96];
    const int length = std::snprintf(
        response, sizeof(response),
        "{\"physical_address\":%u,\"area\":%u,\"line\":%u,\"member\":%u,\"running\":%s}",
        address, knx_physical_area(address), knx_physical_line(address),
        knx_physical_member(address),
        state == ESP_KNX_IP_STATE_RUNNING ? "true" : "false");
    if (length < 0 || static_cast<size_t>(length) >= sizeof(response)) {
        return send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Unable to format KNX configuration");
    }
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_send(request, response, length);
}

esp_err_t set_config(httpd_req_t *request)
{
    constexpr size_t kMaximumBodyLength = 63;
    if (request->content_len <= 0 ||
        static_cast<size_t>(request->content_len) > kMaximumBodyLength) {
        return send_error(request, HTTPD_400_BAD_REQUEST,
                          "Invalid request body");
    }

    char body[kMaximumBodyLength + 1] = {};
    size_t received = 0;
    while (received < static_cast<size_t>(request->content_len)) {
        const int chunk = httpd_req_recv(request, body + received,
                                         request->content_len - received);
        if (chunk == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (chunk <= 0) {
            return send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                              "Unable to read request body");
        }
        received += static_cast<size_t>(chunk);
    }
    body[received] = '\0';

    char value[8] = {};
    if (httpd_query_key_value(body, "physical_address", value,
                              sizeof(value)) != ESP_OK) {
        return send_error(request, HTTPD_400_BAD_REQUEST,
                          "physical_address is required");
    }
    errno = 0;
    char *end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 0);
    if (errno != 0 || end == value || *end != '\0' || parsed > UINT16_MAX) {
        return send_error(request, HTTPD_400_BAD_REQUEST,
                          "physical_address must be a 16-bit integer");
    }

    auto handle = static_cast<esp_knx_ip_handle_t>(request->user_ctx);
    const esp_err_t result = esp_knx_ip_set_physical_address(
        handle, static_cast<knx_address_t>(parsed), true);
    if (result != ESP_OK) {
        return send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Unable to persist KNX configuration");
    }
    httpd_resp_set_status(request, "204 No Content");
    return httpd_resp_send(request, nullptr, 0);
}

}  // namespace

extern "C" esp_err_t esp_knx_ip_web_register(httpd_handle_t server,
                                              esp_knx_ip_handle_t knx_handle)
{
    if (server == nullptr || knx_handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    const httpd_uri_t get_uri = {
        .uri = ESP_KNX_IP_WEB_CONFIG_URI,
        .method = HTTP_GET,
        .handler = get_config,
        .user_ctx = knx_handle,
    };
    esp_err_t result = httpd_register_uri_handler(server, &get_uri);
    if (result != ESP_OK) {
        return result;
    }

    const httpd_uri_t post_uri = {
        .uri = ESP_KNX_IP_WEB_CONFIG_URI,
        .method = HTTP_POST,
        .handler = set_config,
        .user_ctx = knx_handle,
    };
    result = httpd_register_uri_handler(server, &post_uri);
    if (result != ESP_OK) {
        httpd_unregister_uri_handler(server, ESP_KNX_IP_WEB_CONFIG_URI,
                                     HTTP_GET);
    }
    return result;
}

extern "C" esp_err_t esp_knx_ip_web_unregister(httpd_handle_t server)
{
    if (server == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_err_t result = httpd_unregister_uri_handler(
        server, ESP_KNX_IP_WEB_CONFIG_URI, HTTP_GET);
    const esp_err_t post_result = httpd_unregister_uri_handler(
        server, ESP_KNX_IP_WEB_CONFIG_URI, HTTP_POST);
    return result != ESP_OK ? result : post_result;
}
