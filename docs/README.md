# ESP8266 Server Docs

## Scope
This folder documents the ESP8266 TCP sensor server modules in `src/`.

## Module Map
- main.cpp: TCP server lifecycle, command dispatch, ISR hook.
- login.cpp: WiFi join, SSD1306 display init, HTTP device registration.
- cryptography.cpp: AES-128-CBC encrypt/decrypt helpers, LittleFS key/IV readers, WiFi credential decryption.
- sensors.cpp: Sensor discovery, I2C scan mapping, sensor payload build and CRC/encrypt path.
- oneWire.cpp: DS18B20 one-wire detection and temperature reads.
- system.cpp: BLK/RST command actions.
- setStatic.cpp: legacy static-IP experiment (currently commented out).

## Docs Index
- main.md: module-level notes for server entry point and request handling.
- login.md: WiFi join, display init, and backend registration.
- cryptography.md: AES helpers, global buffers, LittleFS key/IV I/O, credential decrypt flow.
- sensors.md: sensor discovery and payload formation.
- oneWire.md: DS18B20 read flow and return codes.
- system.md: BLK/RST command effects.
- setStatic.md: archived static-IP design notes.

## Protocol Summary
- TCP port: 8888
- Sensor read command: `ALL` (case-insensitive — received command is uppercased)
- Control commands: `BLK`, `RST` (handled by `performSystemTask()`)
- Sensor response format (AES on): `<crc32_hex>:<ciphertext>:<iv_hex_csv>`
- Sensor response format (`SOCKET_AE`): `<crc32_hex>:<plaintext>`
- Non-`ALL` response format: `<cmd>_<server_IPv4>`

## Runtime Sequence
1. `setup()` scans/initializes sensors via `configSensors()`.
2. If sensors found: starts `WiFiServer` then joins WiFi and registers with backend.
3. If no sensors found: prints `No Device Found check wiring`; server never starts.
4. Always: configures `LED_BUILTIN` output and attaches ISR to `D6` (`INPUT_PULLUP`, `CHANGE` edge).
5. `loop()` accepts one client at a time; 5-second inactivity timeout; reads command (uppercased, max 79 chars).
6. If command contains `ALL`: calls `getSensorData()` and returns encrypted/CRC payload.
7. Otherwise: returns `<cmd>_<server_IP>` and calls `performSystemTask()`.

## Notes
- Socket payload may be AES/base64 encrypted depending on compile flags (`SOCKET_AE`).
- ISR (`isr()`) attached to `D6` on `CHANGE` edge: blinks `LED_BUILTIN` once and prints to Serial. Serial and `delay()` in ISR context are unsafe and intended for testing only.
