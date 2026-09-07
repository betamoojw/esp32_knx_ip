#include "esp_knx_ip_internal/transport.hpp"

#include "esp_knx_ip_internal/protocol.hpp"

#include "esp_log.h"
#include "lwip/sockets.h"
#include "sdkconfig.h"

#include <cerrno>
#include <sys/time.h>

#ifndef CONFIG_ESP_KNX_IP_PACKET_BUFFER_SIZE
#define CONFIG_ESP_KNX_IP_PACKET_BUFFER_SIZE 512
#endif

#ifndef CONFIG_ESP_KNX_IP_TASK_STACK_SIZE
#define CONFIG_ESP_KNX_IP_TASK_STACK_SIZE 4096
#endif

#ifndef CONFIG_ESP_KNX_IP_TASK_PRIORITY
#define CONFIG_ESP_KNX_IP_TASK_PRIORITY 5
#endif

namespace esp_knx_ip::internal {
namespace {

constexpr char kTag[] = "esp_knx_ip";

void receive_task(void *argument)
{
    auto *context = static_cast<esp_knx_ip_context *>(argument);
    uint8_t packet[CONFIG_ESP_KNX_IP_PACKET_BUFFER_SIZE];

    for (;;) {
        lock(context);
        const bool stopping = context->state != ESP_KNX_IP_STATE_RUNNING;
        unlock(context);
        if (stopping) {
            break;
        }

        const int received = recvfrom(context->socket, packet, sizeof(packet),
                                      0, nullptr, nullptr);
        if (received < 0) {
            lock(context);
            const bool stopped_after_receive =
                context->state != ESP_KNX_IP_STATE_RUNNING;
            unlock(context);
            if (stopped_after_receive) {
                break;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            ESP_LOGW(kTag, "UDP receive failed: errno=%d", errno);
            continue;
        }

        knx_telegram_t telegram = {};
        const ParseResult result = parse_routing_indication(
            packet, static_cast<size_t>(received), &telegram);
        if (result == ParseResult::kOk) {
            dispatch_telegram(context, telegram);
        } else if (result != ParseResult::kUnsupportedService &&
                   result != ParseResult::kUnsupportedMessage) {
            ESP_LOGD(kTag, "Rejected KNXnet/IP packet: parse result=%d",
                     static_cast<int>(result));
        }
    }

    xEventGroupSetBits(context->events, kTaskStoppedBit);
    vTaskDelete(nullptr);
}

void close_socket(esp_knx_ip_context *context)
{
    if (context->socket >= 0) {
        close(context->socket);
        context->socket = -1;
    }
}

}  // namespace

esp_err_t transport_open(esp_knx_ip_context *context)
{
    esp_netif_ip_info_t ip_info = {};
    esp_err_t result = esp_netif_get_ip_info(context->netif, &ip_info);
    if (result != ESP_OK) {
        return result;
    }
    if (ip_info.ip.addr == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    context->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (context->socket < 0) {
        ESP_LOGE(kTag, "Failed to create UDP socket: errno=%d", errno);
        return ESP_FAIL;
    }

    const int reuse_address = 1;
    timeval receive_timeout = {};
    receive_timeout.tv_usec = 250000;
    timeval send_timeout = {};
    send_timeout.tv_usec = 250000;
    if (setsockopt(context->socket, SOL_SOCKET, SO_REUSEADDR,
                   &reuse_address, sizeof(reuse_address)) < 0 ||
        setsockopt(context->socket, SOL_SOCKET, SO_RCVTIMEO,
                   &receive_timeout, sizeof(receive_timeout)) < 0 ||
        setsockopt(context->socket, SOL_SOCKET, SO_SNDTIMEO,
                   &send_timeout, sizeof(send_timeout)) < 0) {
        ESP_LOGE(kTag, "Failed to configure UDP socket: errno=%d", errno);
        close_socket(context);
        return ESP_FAIL;
    }

    sockaddr_in local_endpoint = {};
    local_endpoint.sin_family = AF_INET;
    local_endpoint.sin_port = htons(context->port);
    local_endpoint.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(context->socket, reinterpret_cast<sockaddr *>(&local_endpoint),
             sizeof(local_endpoint)) < 0) {
        ESP_LOGE(kTag, "Failed to bind UDP port %u: errno=%d",
                 context->port, errno);
        close_socket(context);
        return ESP_FAIL;
    }

    context->multicast_endpoint = {};
    context->multicast_endpoint.sin_family = AF_INET;
    context->multicast_endpoint.sin_port = htons(context->port);
    if (inet_aton(context->multicast_address,
                  &context->multicast_endpoint.sin_addr) == 0) {
        close_socket(context);
        return ESP_ERR_INVALID_ARG;
    }

    context->multicast_membership = {};
    context->multicast_membership.imr_multiaddr =
        context->multicast_endpoint.sin_addr;
    context->multicast_membership.imr_interface.s_addr = ip_info.ip.addr;
    if (setsockopt(context->socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   &context->multicast_membership,
                   sizeof(context->multicast_membership)) < 0) {
        ESP_LOGE(kTag, "Failed to join multicast group: errno=%d", errno);
        close_socket(context);
        return ESP_FAIL;
    }

    in_addr outgoing_interface = {};
    outgoing_interface.s_addr = ip_info.ip.addr;
    const uint8_t multicast_ttl = 1;
    if (setsockopt(context->socket, IPPROTO_IP, IP_MULTICAST_IF,
                   &outgoing_interface, sizeof(outgoing_interface)) < 0 ||
        setsockopt(context->socket, IPPROTO_IP, IP_MULTICAST_TTL,
                   &multicast_ttl, sizeof(multicast_ttl)) < 0) {
        ESP_LOGE(kTag, "Failed to configure multicast output: errno=%d", errno);
        setsockopt(context->socket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                   &context->multicast_membership,
                   sizeof(context->multicast_membership));
        close_socket(context);
        return ESP_FAIL;
    }
    return ESP_OK;
}

void transport_close(esp_knx_ip_context *context)
{
    if (context->socket < 0) {
        return;
    }
    if (setsockopt(context->socket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                   &context->multicast_membership,
                   sizeof(context->multicast_membership)) < 0) {
        ESP_LOGW(kTag, "Failed to leave multicast group: errno=%d", errno);
    }
    close_socket(context);
}

esp_err_t transport_start_receive_task(esp_knx_ip_context *context)
{
    if (xTaskCreate(receive_task, "knx_ip_rx",
                    CONFIG_ESP_KNX_IP_TASK_STACK_SIZE, context,
                    CONFIG_ESP_KNX_IP_TASK_PRIORITY, &context->task) != pdPASS) {
        context->task = nullptr;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void transport_wake_receiver(esp_knx_ip_context *context)
{
    if (context->socket >= 0 && shutdown(context->socket, SHUT_RDWR) < 0) {
        ESP_LOGD(kTag, "UDP shutdown returned errno=%d", errno);
    }
}

esp_err_t transport_send(esp_knx_ip_context *context,
                         const sockaddr_in &endpoint,
                         knx_address_t group_address,
                         knx_command_t command,
                         const uint8_t *data,
                         size_t data_length)
{
    uint8_t packet[CONFIG_ESP_KNX_IP_PACKET_BUFFER_SIZE];
    size_t packet_length = 0;

    lock(context);
    if (context->state != ESP_KNX_IP_STATE_RUNNING) {
        unlock(context);
        return ESP_ERR_INVALID_STATE;
    }
    if (!build_routing_indication(context->physical_address, group_address,
                                  command, data, data_length, packet,
                                  sizeof(packet), &packet_length)) {
        unlock(context);
        return ESP_ERR_INVALID_ARG;
    }
    const int sent = sendto(
        context->socket, packet, packet_length, 0,
        reinterpret_cast<const sockaddr *>(&endpoint), sizeof(endpoint));
    unlock(context);
    if (sent < 0 || static_cast<size_t>(sent) != packet_length) {
        ESP_LOGE(kTag, "Failed to send KNX telegram: errno=%d", errno);
        return ESP_FAIL;
    }
    return ESP_OK;
}

}  // namespace esp_knx_ip::internal
