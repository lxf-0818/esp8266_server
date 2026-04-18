# ESP8266 Server Docs

## Scope
This folder documents the ESP8266 TCP sensor server modules in `src/`.

## Module Map
- main.cpp: TCP server lifecycle, command dispatch, ISR hook.
- login.cpp: LittleFS credential decrypt, WiFi join, AES helpers, HTTP device registration.
- sensors.cpp: Sensor discovery, I2C scan mapping, sensor payload build and CRC/encrypt path.
- oneWire.cpp: DS18B20 one-wire detection and temperature reads.
- system.cpp: BLK/RST command actions.
- setStatic.cpp: legacy static-IP experiment (currently commented out).

## Docs Index
- main.md: module-level notes for server entry point and request handling.
- login.md: WiFi/AES/LittleFS and backend update behavior.
- sensors.md: sensor discovery and payload formation.
- oneWire.md: DS18B20 read flow and return codes.
- system.md: BLK/RST command effects.
- setStatic.md: archived static-IP design notes.

## Protocol Summary
- TCP port: 8888
- Sensor read command: ALL
- Control commands: BLK, RST
- Sensor response format: `<crc32_hex>:<payload>`

## Runtime Sequence
1. `setup()` scans/initializes sensors.
2. Starts `WiFiServer` and joins WiFi.
3. Registers IP/MAC/sensor list with local PHP endpoint.
4. `loop()` accepts one client request, responds, and closes socket.

## Notes
- Socket payload may be AES/base64 encrypted depending on compile flags.
- ISR currently does serial and delay work, which is generally unsafe in interrupt context. (testing only)
