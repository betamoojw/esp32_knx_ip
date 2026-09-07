# Native ESP-IDF basic example

Configure the target and Wi-Fi credentials with `idf.py menuconfig`, then flash
the example. The application owns Wi-Fi and NVS initialization; the KNX
component receives only the connected station `esp_netif_t`.

The example registers the persisted mapping `temperature` with default group
address `1/2/3`, decodes incoming DPT9 payloads, and sends `21.5` as a KNX group
write after startup. Set a nonzero example run time in menuconfig to exercise
clean component shutdown; zero keeps the firmware running continuously.
