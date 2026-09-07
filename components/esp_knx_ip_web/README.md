# ESP KNX/IP web adapter

Optional `esp_http_server` adapter for the public `esp_knx_ip` API. Applications
that need HTTP configuration add `esp_knx_ip_web` to their component
requirements and include `esp_knx_ip_web/esp_knx_ip_webserver.h`.

The adapter registers `GET` and `POST` handlers at `/knx/config`. It does not
create, start, stop, or own the HTTP server, and it does not access KNX private
state or NVS directly.
