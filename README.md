# Native ESP-IDF KNXnet/IP component

This repository ports
[`betamoojw/ESP32_KNX_IP_Library_fix2bytefloat`](https://github.com/betamoojw/ESP32_KNX_IP_Library_fix2bytefloat)
from Arduino to a native ESP-IDF component. It provides KNXnet/IP routing over
UDP multicast `224.0.23.12:3671`, optional unicast transmission, bounded cEMI
parsing, DPT conversion, callbacks, NVS persistence, and an optional
`esp_http_server` configuration adapter.

The component does not initialize Wi-Fi, Ethernet, the default event loop, or
NVS. Those resources belong to the application. See
[the component documentation](components/esp_knx_ip/README.md) and the
[native example](examples/basic/README.md).

## Layout

```text
components/
|-- esp_knx_ip/               KNX runtime, protocol, DPT, NVS, and transport
`-- esp_knx_ip_web/           Optional esp_http_server adapter
examples/
`-- basic/                    Standalone native ESP-IDF application
```

## Integration

Copy `components/esp_knx_ip` into an ESP-IDF application's `components`
directory, include `esp_knx_ip/esp_knx_ip.h`, and add `esp_knx_ip` to the
application component's `REQUIRES` list. Configuration is available under
`Component config -> ESP KNX/IP component`.

Applications requiring HTTP configuration also copy `esp_knx_ip_web`, add it
to `REQUIRES`, and include `esp_knx_ip_web/esp_knx_ip_webserver.h`.

The implementation targets ESP-IDF 5.x and ESP32-family targets with IPv4,
lwIP sockets, FreeRTOS, `esp_netif`, and NVS support.

## Validation status

Deterministic host and ESP-IDF Unity tests are included. They have not been
executed in this workspace because no host C++ toolchain or configured ESP-IDF
environment is available, and validation commands were intentionally not run.