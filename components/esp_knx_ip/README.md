# ESP KNX/IP

`esp_knx_ip` is a native ESP-IDF C/C++ component for KNXnet/IP routing. It has
no Arduino or PlatformIO dependency and does not own application connectivity.

## Architecture

```text
Application
|-- initializes NVS
|-- initializes Wi-Fi or Ethernet and obtains esp_netif_t
|-- optionally owns esp_http_server
`-- esp_knx_ip handle
    |-- versioned NVS configuration
    |-- UDP socket and multicast membership
    |-- FreeRTOS receive task
    |-- bounded KNXnet/IP and cEMI parser
    |-- group-address callback registry
    `-- portable DPT codecs
```

The handle owns its socket, mutex, event group, receive task, and callback
registry. The application owns `esp_netif_t`, NVS initialization, connectivity,
and any HTTP server passed to the separate `esp_knx_ip_web` component.

Public headers live under `include/esp_knx_ip`. Protocol, persistence,
transport, and handle-state contracts live under
`private_include/esp_knx_ip_internal` and are not application API.

## Dependencies

| Arduino implementation | Native implementation |
|---|---|
| `Arduino.h`, global `knx` object | Explicit `esp_knx_ip_handle_t` |
| `WiFi`, credentials in sketch | Application-owned `esp_netif` and Wi-Fi/Ethernet |
| `WiFiUDP` | lwIP BSD sockets and `IP_ADD_MEMBERSHIP` |
| `EEPROM` | Versioned NVS namespace `esp_knx_ip` |
| `WebServer` | Optional application-owned `esp_http_server` |
| `String` | Fixed buffers and caller-owned data |
| `knx.loop()` | Dedicated FreeRTOS receive task |
| `Serial` debug macros | ESP-IDF logging |
| Header `#define` configuration | Kconfig |

## Setup

The application must initialize NVS and establish IP connectivity first:

```cpp
esp_netif_t *netif = /* connected Wi-Fi or Ethernet interface */;
esp_knx_ip_config_t config = ESP_KNX_IP_CONFIG_DEFAULT(netif);
config.physical_address = knx_physical_address(1, 1, 1);

esp_knx_ip_handle_t knx = nullptr;
ESP_ERROR_CHECK(esp_knx_ip_create(&config, &knx));
ESP_ERROR_CHECK(esp_knx_ip_register_callback(
    knx, knx_group_address(1, 2, 3), on_telegram, user_context));
ESP_ERROR_CHECK(esp_knx_ip_start(knx));
```

Use `esp_knx_ip_stop()` before destroying a running handle. A stopped handle can
be started again. On an IP change or reconnect, stop and restart the handle so
the multicast membership is joined on the interface's current address.

## Sending and receiving

`esp_knx_ip_send()` sends a routing indication to the configured multicast
endpoint. `esp_knx_ip_send_unicast()` sends the same validated frame to an
explicit IPv4 endpoint. Both calls are thread-safe and serialize with shutdown.

Callbacks execute in the component receive task with stack size and priority
selected by Kconfig. Callback data is valid only for the callback duration.
Callbacks may send telegrams and update configuration, but must not block for
long periods and must not call `esp_knx_ip_stop()` or destroy the handle.

Only one callback is stored per group address; registering the same address
again replaces it. Other callback registrations remain active concurrently.
Use `esp_knx_ip_register_persistent_callback()` with a stable name of up to 13
characters when the group address must be configurable. It loads the saved NVS
mapping or uses the supplied default. Update that mapping with
`esp_knx_ip_set_registered_group_address()`.

The `knx_telegram_t::data` field is the APDU data beginning with the lower six
bits of the first APCI/data byte. Multi-byte DPT payloads therefore begin at
`telegram.data + 1`. Likewise, callers prepend a zero byte before passing a
multi-byte encoded DPT payload to `esp_knx_ip_send()`.

## DPT conversion

The public, allocation-free codec covers the following DPT main families. A
`Full` family validates its defined wire structure and ranges. `Wire-level`
means the family bytes can be encoded and decoded, but the caller must apply
the selected subtype's enum, reserved-value, or named-bit semantics.

| DPT | KNX value | Bytes | Coverage |
|---|---|---:|---|
| 1 | Boolean | 1 compact bit | Full; all DPT 1 subtypes share this encoding |
| 2 | 1-bit controlled | 1 compact byte | Full |
| 3 | 3-bit controlled | 1 compact byte | Full |
| 4 | Character | 1 | Full for DPT 4.001 ASCII through `knx_dpt4_ascii_*`; raw family API supports DPT 4.002 ISO-8859-1 |
| 5 | Unsigned 8-bit | 1 | Full raw U8, DPT 5.001 scaling, and DPT 5.003 angle conversions |
| 6 | Signed 8-bit | 1 | Full numeric encoding; DPT 6.020 status/mode fields are wire-level |
| 7 | Unsigned 16-bit | 2 | Full; subtype units share the family encoding |
| 8 | Signed 16-bit | 2 | Full; subtype units share the family encoding |
| 9 | KNX 16-bit float | 2 | Full with fork-compatible encoder behavior described below |
| 10 | Time of day | 3 | Full, including weekday and reserved-bit validation |
| 11 | Date | 3 | Full with leap-year calendar validation |
| 12 | Unsigned 32-bit | 4 | Full; subtype units share the family encoding |
| 13 | Signed 32-bit | 4 | Full; subtype units share the family encoding |
| 14 | IEEE-754 binary32 | 4 | Full; subtype units share the family encoding |
| 15 | Access data | 4 | Wire-level packed value; field credentials and error semantics are caller-owned |
| 16 | Fixed string | 14 | Full family storage; caller selects ASCII or ISO-8859-1 character policy |
| 17 | Scene number | 1 | Full, scene 0 through 63 |
| 18 | Scene control | 1 | Full learn/recall flag and scene number |
| 19 | Date and time | 8 | Full structural, calendar, validity-flag, and reserved-bit handling |
| 20 | 8-bit enumeration | 1 | Wire-level; valid enum members depend on the subtype |
| 21 | 8-bit status set | 1 | Wire-level; named and reserved bits depend on the subtype |
| 22 | 16-bit status set | 2 | Wire-level; named and reserved bits depend on the subtype |
| 23 | 2-bit control | 1 compact byte | Full family wire shape; action meaning depends on the subtype |
| 24 | Variable 8-bit string | Variable, NUL-terminated | Full framing; character interpretation is caller-owned |
| 25 | Double nibble | 1 | Wire-level; nibble meanings depend on the subtype |
| 26 | Scene information | 1 | Full active flag, reserved bit, and scene number |
| 27 | Combined status | 4 | Wire-level 16-bit value plus 16-bit mask |
| 28 | UTF-8 string | Variable, NUL-terminated | Full framing and UTF-8 validation |
| 29 | Signed 64-bit | 8 | Full |
| 30 | 8-bit bitfield | 1 | Wire-level; named and reserved bits depend on the subtype |
| 31 | 24-bit bitfield | 3 | Wire-level; named and reserved bits depend on the subtype |
| 232 | RGB color | 3 | Full raw RGB channels |
| 234 | ISO 639-1 language | 2 | Full lowercase two-letter language code wire format |
| 251 | RGBW color | 6 | Full channels, reserved byte, and low-nibble validity mask |

DPT 17 is a one-byte scene number, not a three-byte value. DPT 234.001 is an
ISO 639-1 language code, not a scene number. These definitions supersede the
incorrect labels in the repository task document.

The component deliberately has no DPT registry or automatic subtype dispatch.
Applications choose the codec matching their configured group object. The
transport remains DPT-agnostic, so every supported payload can be used for
read, write, or response telegrams without coupling protocol parsing to a type
database. There is no MCP or DPT-aware JSON configuration layer in this
repository; the optional HTTP adapter only manages KNX runtime configuration.

All codec functions validate pointers and minimum buffer lengths. Fixed-width
decoders consume the first value when a larger buffer is supplied. Variable
strings require their NUL terminator within the supplied length and report the
encoded length to the caller.

The DPT9 encoder intentionally preserves the fork's fixed magnitude, exponent,
rounding, and 11-bit two's-complement algorithm for values it can emit. Unlike
the fork, it rejects non-finite and overflowing values instead of wrapping the
four-bit exponent. Consequently:

- Encoder range: `-670760.96` through `670760.96`.
- Decoder range: standard KNX DPT9 range `-671088.64` through `670760.96`.
- The standard negative endpoint `F8 00` decodes correctly.
- The fork's unreachable replacement decoder was not copied; the active fork
  decoder mishandled negative values, so receive-side decoding is intentionally
  corrected and regression-tested.

## Persistence

The physical address is stored as `phys_addr` and named group addresses use
`g_<name>` keys in NVS namespace `esp_knx_ip`; the configuration schema version
is `1`. Call `nvs_flash_init()` before creating a handle when NVS loading is
enabled. `esp_knx_ip_set_physical_address(..., true)` and
`esp_knx_ip_set_registered_group_address(..., true)` write and commit values.
Callback functions remain application code and must be registered on each boot.

The Arduino EEPROM image is not migrated automatically. Its raw layout embeds
compile-time capacities and callback IDs whose meaning depends on registration
order; importing it without the original firmware schema would be unsafe.

## Optional HTTP adapter

Add the separate `esp_knx_ip_web` component to the application, create an HTTP
server in the application, include
`esp_knx_ip_web/esp_knx_ip_webserver.h`, then call
`esp_knx_ip_web_register()`. The adapter registers:

- `GET /knx/config`: current physical address and running state as JSON.
- `POST /knx/config`: bounded form body such as
  `physical_address=0x1234`; the value is persisted to NVS.

Call `esp_knx_ip_web_unregister()` before stopping the application-owned HTTP
server or destroying the KNX handle. The adapter deliberately excludes the
original generated Bootstrap page and arbitrary application settings from the
protocol core.

## Kconfig

- `ESP_KNX_IP_MAX_GROUP_ADDRESSES`: fixed callback table capacity.
- `ESP_KNX_IP_PACKET_BUFFER_SIZE`: receive/send stack buffer, minimum 271.
- `ESP_KNX_IP_TASK_STACK_SIZE`: receive task stack bytes.
- `ESP_KNX_IP_TASK_PRIORITY`: receive task priority.

Runtime configuration controls the network interface, physical address,
multicast IPv4 address, UDP port, and whether to load the physical address from
NVS.

## Migration

| Arduino API | Native ESP-IDF API |
|---|---|
| `knx.load()` | Initialize NVS, then create with NVS loading enabled |
| `knx.start()` | `esp_knx_ip_create()` then `esp_knx_ip_start()` |
| `knx.loop()` | Removed; receive task is internal |
| `knx.callback_register()` plus `callback_assign()` | `esp_knx_ip_register_callback()` |
| EEPROM-backed callback assignment | `esp_knx_ip_register_persistent_callback()` |
| `knx.physical_address_set()` | `esp_knx_ip_set_physical_address()` |
| `knx.send()` | `esp_knx_ip_send()` or `esp_knx_ip_send_unicast()` |
| `knx.send_2byte_float()` | `knx_dpt9_encode()`, prepend APDU byte, then send |
| `knx.data_to_2byte_float()` | `knx_dpt9_decode()` |
| Built-in Arduino web page | Optional `esp_knx_ip_web_register()` REST adapter |
| Implicit global lifetime | Explicit stop and destroy |

## Tests

The portable test at `test/host/test_protocol.cpp` checks address conversion,
known packet bytes, malformed packets, DPT round trips, and exact fork DPT9
encoder vectors. It can be compiled with any C++17 compiler by including
`include` and `private_include` and linking `knx_dpt.cpp` and
`knx_ip_protocol.cpp`.

`test/test_esp_knx_ip.cpp` is an ESP-IDF Unity component covering DPT9,
protocol parsing, and NVS persistence. Run it through ESP-IDF's unit-test-app
workflow. Hardware integration should additionally verify multicast join,
rejoin after an address change, multicast and unicast traffic against a KNX/IP
router, callback dispatch, and repeated start/stop under load.

## Error handling and limits

Malformed headers, lengths, cEMI message types, non-group destinations, and
oversized APDUs are rejected before copying. Public lifecycle and transport
operations return `esp_err_t`; parser and codec helpers do not allocate.

This port implements KNXnet/IP routing indications. It does not implement KNX/IP
tunneling connection management, routing-busy/lost messages, IPv6 multicast,
automatic interface event handling, or migration of arbitrary Arduino custom
configuration and feedback widgets.