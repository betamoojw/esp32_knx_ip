# AGENTS.md

## Task: Port `ESP32_KNX_IP_Library_fix2bytefloat` to a Native ESP-IDF Component

### Objective

Review the existing PlatformIO/Arduino library:

- Repository: https://github.com/betamoojw/ESP32_KNX_IP_Library_fix2bytefloat
- Purpose: KNXnet/IP communication on ESP32
- Current implementation: Arduino/PlatformIO-oriented
- KNXnet/IP transport: UDP multicast `224.0.23.12:3671`

Convert this library into a **clean, native ESP-IDF component** that can be integrated into ESP-IDF applications without requiring the Arduino framework or Arduino compatibility layer.

The result must preserve the existing KNXnet/IP functionality and the existing `2-byte float` behavior/fix, while replacing Arduino-specific dependencies and architecture with idiomatic ESP-IDF APIs and component conventions.

---

## 1. Repository Review

Before modifying code:

1. Inspect the complete source tree, including:
   - `esp-knx-ip.h`
   - `esp-knx-ip.cpp`
   - `esp-knx-ip-config.cpp`
   - `esp-knx-ip-conversion.cpp`
   - `esp-knx-ip-send.cpp`
   - `esp-knx-ip-webserver.cpp`
   - `DPT.h`
   - examples
   - `library.properties`
   - README and documentation
2. Identify all Arduino/PlatformIO-specific dependencies and assumptions, including:
   - `Arduino.h`
   - `WiFi`
   - `WiFiUDP` / UDP abstractions
   - `EEPROM`
   - `WebServer` or Arduino web-server APIs
   - `String`
   - `delay()`
   - `millis()`
   - Arduino GPIO/network types
   - Arduino callback conventions
   - PlatformIO-specific build configuration
3. Trace the complete KNXnet/IP data path:
   - network initialization
   - UDP socket creation
   - multicast join
   - multicast receive
   - unicast transmit
   - KNXnet/IP frame parsing
   - cEMI/L_Data handling if applicable
   - group-address extraction
   - DPT encoding/decoding
   - callback dispatch
   - configuration persistence
4. Identify the exact implementation responsible for the `2-byte float` fix and preserve its behavior. Do not silently replace it with a different implementation unless equivalence is demonstrated by tests.

Do not begin the port by blindly translating Arduino APIs line-by-line. First understand the architecture and define the ESP-IDF component boundaries.

---

## 2. Target Architecture

The repository is a reusable ESP-IDF component repository, not an application.
Application projects consume the components from `components/`; independently
buildable demonstrations live under the repository-level `examples/` directory.

Use the following structure as the architectural source of truth:

```text
components/
├── esp_knx_ip/                     Reusable KNXnet/IP runtime and protocol
│   ├── CMakeLists.txt
│   ├── Kconfig
│   ├── include/esp_knx_ip/         Installed public API only
│   │   ├── esp_knx_ip.h
│   │   ├── knx_dpt.h
│   │   └── knx_types.h
│   ├── private_include/esp_knx_ip_internal/
│   │   ├── context.hpp             Private handle state and registry
│   │   ├── config.hpp              NVS persistence contract
│   │   ├── protocol.hpp            KNXnet/IP/cEMI codec contract
│   │   └── transport.hpp           Socket/task contract
│   ├── src/
│   │   ├── esp_knx_ip.cpp          Public lifecycle and callback facade
│   │   ├── knx_dpt.cpp             Allocation-free DPT codecs
│   │   ├── knx_ip_config.cpp       Versioned NVS implementation
│   │   ├── knx_ip_protocol.cpp     Platform-independent frame codec
│   │   └── knx_ip_transport.cpp    lwIP and FreeRTOS transport
│   ├── test/                        ESP-IDF Unity component tests
│   └── README.md
├── esp_knx_ip_web/                 Optional HTTP configuration adapter
│   ├── CMakeLists.txt
│   ├── include/esp_knx_ip_web/
│   │   └── esp_knx_ip_webserver.h
│   ├── src/esp_knx_ip_webserver.cpp
│   └── README.md
examples/
└── basic/                          Standalone ESP-IDF application
      ├── CMakeLists.txt
      ├── Kconfig.projbuild
      ├── README.md
      └── main/
            ├── CMakeLists.txt
            └── main.cpp
```

### Component responsibilities

- `esp_knx_ip` owns KNX handle lifecycle, callback registration, UDP
   multicast/unicast transport, cEMI framing, DPT conversion, and KNX-related
   NVS data. It accepts an already-connected `esp_netif_t`; it never initializes
   Wi-Fi, Ethernet, the default event loop, credentials, or the NVS partition.
- `esp_knx_ip_web` adapts an application-owned `esp_http_server` to the public
   KNX API. It must not include private KNX headers, access sockets or NVS
   directly, or be required by applications that only need KNX transport.
- `examples/basic` is application code. It owns networking, credentials, event
   handlers, NVS initialization, and top-level startup/shutdown sequencing.
- `esp_knx_ip/test` may test private contracts through `PRIV_INCLUDE_DIRS`, but
   production consumers may include only headers under `include/`.

### Dependency direction

```text
examples/basic -> esp_knx_ip -> esp_netif, nvs_flash, FreeRTOS, lwIP
optional application -> esp_knx_ip_web -> esp_knx_ip, esp_http_server
```

Dependencies flow only in the directions shown. `esp_knx_ip` must not depend on
`esp_wifi`, Ethernet drivers, `esp_http_server`, application configuration, or
the example. Protocol and DPT code must remain independent of sockets, tasks,
NVS, and hardware-specific drivers so it can be tested deterministically.

### Build and configuration boundaries

- Every component uses `idf_component_register()` with explicit source files,
   public include directories, private include directories, and direct
   `REQUIRES`/`PRIV_REQUIRES` dependencies.
- `Kconfig` contains only component resource and feature limits. Network
   endpoints, physical/group addresses, and interface selection are runtime
   configuration. Application credentials stay in the example's
   `Kconfig.projbuild`, never in a reusable component.
- Optional functionality with an independent dependency graph is a separate
   component rather than conditional public headers or conditional component
   dependencies.
- Public headers are C-compatible, stable, and free of private structures.
   Internal C++ headers live under `private_include` and are never installed as
   application API.
- Packet-processing paths use fixed-capacity buffers and explicit ownership;
   heap allocation is limited to handle creation and ESP-IDF resource creation.

The architecture must build natively with ESP-IDF, contain no Arduino or
PlatformIO dependency, and remain portable across ESP32-family targets that
provide IPv4 lwIP sockets, FreeRTOS, `esp_netif`, and NVS.

---

## 3. Replace Arduino Dependencies

Map Arduino functionality to ESP-IDF equivalents.

### Networking

Replace Arduino Wi-Fi/UDP abstractions with ESP-IDF networking APIs:

- `esp_netif`
- `esp_wifi` where Wi-Fi is required by the application
- BSD/POSIX sockets
- `lwIP`
- `setsockopt()`
- multicast socket APIs
- appropriate interface binding and multicast membership handling

The KNX component should **not own application Wi-Fi credentials or unnecessarily initialize the Wi-Fi driver**. It should operate on an already-connected network interface and provide a clean initialization/start API.

Clearly define the ownership boundary between:

```text
Application
    |
    +-- Wi-Fi / Ethernet initialization
    |
    +-- IP connectivity
    |
    +-- esp_knx_ip component
             |
             +-- UDP multicast/unicast
             +-- KNXnet/IP protocol
             +-- DPT conversion
             +-- callbacks
```

### Timing

Replace:

- `delay()`
- `millis()`

with appropriate FreeRTOS/ESP-IDF mechanisms such as:

- `vTaskDelay()`
- `esp_timer_get_time()`
- FreeRTOS timers where appropriate.

Avoid unnecessary blocking delays.

### Strings and memory

Replace Arduino `String` and implicit dynamic allocation with:

- `std::string` where C++ is justified and safe;
- fixed-size buffers;
- `std::string_view` where appropriate;
- explicit ownership rules;
- ESP-IDF heap APIs only where needed.

Avoid heap allocation in high-frequency packet-processing paths unless justified.

### Persistence

Replace Arduino EEPROM usage with **ESP-IDF NVS**.

Define a small persistence layer for:

- KNX physical address;
- configurable group-address mappings;
- component configuration;
- other values currently stored by the Arduino implementation.

Use versioned NVS namespaces/keys and provide migration behavior if the old format is relevant.

---

## 4. Configuration and Kconfig

Move compile-time configuration currently contained in `esp-knx-ip.h` into ESP-IDF configuration where appropriate.

Provide a `Kconfig`/`Kconfig.projbuild` only for options that genuinely belong at build time, for example:

- debug logging;
- protocol diagnostics;
- optional web configuration;
- multicast configuration;
- buffer sizes;
- task stack size;
- task priority;
- queue depth;
- feature switches.

Do not put application-specific configuration into Kconfig if it should be runtime configuration.

Provide sensible defaults.

---

## 5. Public API

Design a clean ESP-IDF-oriented API.

The public interface should support, as applicable:

- component initialization;
- start/stop;
- configuration loading/saving;
- physical address configuration;
- group-address registration;
- callback registration;
- sending KNX group telegrams;
- receiving telegrams;
- DPT conversion;
- querying component status.

Avoid exposing internal socket structures, parser state, or implementation-specific classes.

Prefer an explicit context/handle model if it improves lifecycle management and makes multiple instances possible.

Example style:

```cpp
esp_err_t esp_knx_ip_init(const esp_knx_ip_config_t *config);
esp_err_t esp_knx_ip_start(void);
esp_err_t esp_knx_ip_stop(void);
esp_err_t esp_knx_ip_register_group_address(...);
esp_err_t esp_knx_ip_register_callback(...);
esp_err_t esp_knx_ip_send(...);
```

The final API should be based on the actual source architecture rather than copying this example mechanically.

Document thread-safety and callback execution context.

---

## 6. FreeRTOS and Task Design

Do not simply reproduce the Arduino `knx.loop()` model.

Evaluate whether the ESP-IDF implementation should use:

- a dedicated KNX network task;
- event-driven socket handling;
- a receive queue;
- a timer/task for periodic protocol work.

The design must avoid blocking the main application task.

Document:

- task priority;
- stack size;
- queue sizes;
- timeout behavior;
- shutdown behavior;
- callback context;
- concurrency assumptions.

Callbacks must not execute in an unsafe context if they may perform blocking or application-level operations.

---

## 7. KNXnet/IP Protocol Preservation

Preserve the existing protocol behavior unless a defect is discovered.

Verify at minimum:

1. KNXnet/IP header construction/parsing.
2. UDP multicast address `224.0.23.12`.
3. UDP port `3671`.
4. Multicast group join/leave.
5. Unicast communication.
6. Group address encoding/decoding.
7. Physical address handling.
8. Telegram parsing.
9. Read/write/response semantics implemented by the original library.
10. Callback dispatch.
11. DPT conversion.
12. Existing `2-byte float` implementation/fix.

Do not change protocol semantics merely to make the code easier to port.

If the original implementation contains protocol deviations or interoperability issues, document them separately and fix them only when the correct behavior can be established.

---

## 8. DPT / 2-Byte Float Requirements

The existing repository is specifically named `fix2bytefloat`, so this area is critical.

Identify the exact implementation of the KNX 2-byte floating-point DPT conversion.

Create deterministic unit tests covering:

- positive values;
- negative values;
- zero;
- minimum/maximum representable values;
- rounding;
- exponent handling;
- mantissa handling;
- saturation/overflow;
- boundary values;
- invalid/out-of-range inputs.

Compare the ESP-IDF implementation against the original implementation with a regression test suite.

The port is **not complete** if the KNX 2-byte float behavior changes unintentionally.

---

## 9. Web Configuration

The original library contains web-server functionality.

Determine whether it should remain part of the component.

If retained:

- replace Arduino WebServer APIs with ESP-IDF `esp_http_server`;
- isolate the web layer from the KNX protocol core;
- avoid making the HTTP server mandatory for core KNX operation;
- make it optionally configurable through Kconfig;
- ensure safe request parsing and bounds checking;
- ensure configuration changes are persisted safely through NVS.

Prefer this architecture:

```text
esp_knx_ip
├── protocol core
├── transport
├── configuration/NVS
├── DPT conversion
└── optional HTTP configuration adapter
```

The KNX protocol core must remain usable without the web server.

---

## 10. Error Handling and Logging

Use ESP-IDF conventions:

- `esp_err_t`
- `ESP_OK`
- appropriate error codes
- `ESP_LOGE`
- `ESP_LOGW`
- `ESP_LOGI`
- `ESP_LOGD`

Do not silently ignore socket, memory, parsing, NVS, or configuration errors.

Validate all:

- packet lengths;
- offsets;
- buffer boundaries;
- multicast operations;
- socket return values;
- configuration values;
- callback pointers;
- DPT payload lengths.

Malformed network packets must never cause crashes or out-of-bounds access.

---

## 11. Memory and Reliability

The component should be suitable as a production firmware dependency.

Pay particular attention to:

- stack usage;
- heap usage;
- packet-buffer lifetime;
- callback ownership;
- socket lifecycle;
- task shutdown;
- repeated start/stop;
- network disconnect/reconnect;
- multicast rejoin;
- NVS write frequency;
- memory leaks;
- double initialization;
- race conditions.

Use sanitizing/static-analysis-friendly code where practical.

---

## 12. Tests

Create an ESP-IDF-compatible test strategy.

At minimum, provide:

### Unit tests

- group-address encode/decode;
- physical-address encode/decode;
- KNXnet/IP header parsing;
- packet-length validation;
- DPT conversion;
- 2-byte float conversion;
- configuration serialization/deserialization;
- boundary and malformed-input handling.

### Integration tests

Where practical:

- initialize component;
- start/stop;
- send/receive KNXnet/IP telegram;
- multicast join;
- callback invocation;
- NVS configuration persistence;
- network reconnect behavior.

Tests should be executable through the normal ESP-IDF build/test workflow.

---

## 13. Example Application

Create at least one minimal ESP-IDF example demonstrating:

1. network initialization is performed by the application;
2. KNX component initialization;
3. KNX component start;
4. registration of a group address;
5. registration of a callback;
6. receiving a KNX telegram;
7. decoding a DPT value;
8. sending a KNX telegram;
9. clean shutdown if supported.

The example must build using ESP-IDF only and must not include Arduino headers.

---

## 14. Documentation

Update/create documentation covering:

- component purpose;
- supported ESP-IDF versions;
- supported ESP32 targets;
- installation;
- `idf.py` integration;
- component configuration;
- public API;
- initialization sequence;
- Wi-Fi/Ethernet ownership;
- callback model;
- task/concurrency model;
- NVS persistence;
- DPT support;
- 2-byte float behavior;
- troubleshooting;
- example usage.

Include a migration guide:

```text
Arduino/PlatformIO API
        ->
Native ESP-IDF API
```

For every significant public API that changes, document the old usage and the new equivalent.

---

## 15. Build System

Create a proper ESP-IDF `CMakeLists.txt`.

Use `idf_component_register()` with explicit:

- `SRCS`
- `INCLUDE_DIRS`
- `REQUIRES`
- `PRIV_REQUIRES`

Only declare dependencies actually required.

Do not introduce Arduino as a dependency.

The component should build cleanly with:

```bash
idf.py build
```

and should not require PlatformIO.

---

## 16. Compatibility and Migration Strategy

Prefer a staged migration:

### Phase 1 — Baseline

- Build and understand the original implementation.
- Record current public APIs and behavior.
- Identify the 2-byte float implementation.
- Add regression tests before changing behavior.

### Phase 2 — Protocol Core

- Separate KNXnet/IP protocol logic from Arduino framework code.
- Port packet encoding/decoding and DPT handling.

### Phase 3 — ESP-IDF Transport

- Implement UDP sockets and multicast using ESP-IDF/lwIP.
- Implement task/event architecture.

### Phase 4 — Configuration

- Port persistent configuration to NVS.
- Add Kconfig only where appropriate.

### Phase 5 — Optional Web Layer

- Port web configuration to `esp_http_server`.
- Keep it optional and decoupled.

### Phase 6 — Tests and Example

- Add unit/integration tests.
- Add native ESP-IDF example.

### Phase 7 — Cleanup

- Remove all Arduino/PlatformIO dependencies.
- Run static checks.
- Review public API.
- Update documentation.

---

## 17. Acceptance Criteria

The migration is considered complete only when all of the following are true:

- [ ] The library is a native ESP-IDF component.
- [ ] No Arduino framework dependency remains.
- [ ] No PlatformIO-specific build mechanism is required.
- [ ] `idf_component_register()` is used correctly.
- [ ] The component builds successfully with ESP-IDF.
- [ ] KNXnet/IP multicast/unicast behavior is preserved.
- [ ] Existing KNX group-address functionality is preserved.
- [ ] Existing callback behavior is preserved or clearly documented where intentionally changed.
- [ ] The `2-byte float` fix is preserved and covered by regression tests.
- [ ] Malformed packets are safely rejected.
- [ ] Configuration persistence uses ESP-IDF NVS.
- [ ] Wi-Fi/Ethernet initialization remains application-owned.
- [ ] FreeRTOS task/resource lifecycle is well-defined.
- [ ] Optional web configuration does not contaminate the protocol core.
- [ ] Unit tests cover protocol and DPT conversion.
- [ ] At least one ESP-IDF example builds and runs.
- [ ] Documentation explains installation, configuration, API, migration, and limitations.
- [ ] No known memory leaks, unsafe buffer operations, or uncontrolled task/socket lifecycles remain.

---

## 18. Required Agent Workflow

When executing this task:

1. **Inspect first.** Do not modify code before understanding the complete repository.
2. **Establish a baseline.** Build the original implementation where possible and record its behavior.
3. **Map dependencies.** Produce an Arduino-to-ESP-IDF dependency mapping.
4. **Preserve protocol behavior.** Do not redesign KNX semantics unnecessarily.
5. **Separate concerns.** Keep protocol, transport, configuration, DPT conversion, and web UI logically independent.
6. **Port incrementally.** Keep the project buildable after each major stage.
7. **Test before and after.** Especially test the 2-byte float conversion.
8. **Prefer ESP-IDF-native APIs.** Do not recreate Arduino abstractions unnecessarily.
9. **Do not hide failures.** Report build failures, unsupported behavior, and unresolved compatibility issues explicitly.
10. **Document architectural decisions.**
11. **Run the final build and tests.**
12. **Provide a final migration report** containing:
    - files changed;
    - architecture changes;
    - Arduino dependencies removed;
    - ESP-IDF APIs introduced;
    - public API changes;
    - test results;
    - known limitations;
    - remaining production-readiness work.

### Important Constraint

Do not claim the port is complete merely because the code compiles. The final result must demonstrate **functional equivalence of the KNXnet/IP behavior**, with particular emphasis on the existing **2-byte floating-point DPT implementation**, while providing a maintainable and idiomatic ESP-IDF component architecture.
