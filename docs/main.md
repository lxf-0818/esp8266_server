# Main Server Runtime Documentation

This document describes the behavior of src/main.cpp.

## Purpose

The module is the runtime entry point for the ESP8266 socket server. It:
- initializes serial and sensor configuration
- starts TCP server service on port 8888
- accepts and processes client commands
- returns either sensor snapshots or command echo responses
- triggers system actions for non-ALL commands

## Key Constants and Globals

- `PORT = 8888`
- `WiFiServer server(PORT)`
- `WiFiClient client`
- `cmdFromClient[80]`: incoming command buffer
- `sensorName[100]`: populated by sensor configuration
- `str[80]`, `Buf[80]`: local buffers (limited use in this file)

## Setup Flow

Function: `setup()`

Behavior:
1. Starts serial at 115200.
2. Calls `configSensors(sensorName)` to detect installed sensors.
3. If at least one sensor is present:
   - starts TCP server (`server.begin()`)
   - starts Wi-Fi path (`beginWIFI(sensorName)`)
4. If no sensors are found, logs wiring warning and does not start server/Wi-Fi path.
5. Configures D6 as `INPUT_PULLUP` and built-in LED as output.
6. Attaches interrupt on D6 change to `isr()`.

## Command Handling Flow

Function: `loop()`

Behavior for each accepted client:
1. Accepts incoming client with `server.accept()`.
2. Logs remote IP when connected.
3. Waits up to 5 seconds for command bytes.
4. Reads bytes into `cmdFromClient` with uppercase normalization.
5. If command starts with `ALL`:
   - calls `getSensorData(cmdFromClient, results)`
   - sends payload response to client
6. Otherwise:
   - builds `COMMAND_localIP` echo text
   - sends immediate response
   - runs `performSystemTask(cmdFromClient)`
7. Stops client connection and logs final response.

Buffer handling:
- Incoming command is bounded to `sizeof(cmdFromClient)-1`.
- Excess incoming bytes are discarded.

Timeout behavior:
- If no command arrives within ~5000 ms, the client is closed.

## Protocol Behavior

- Commands are converted to uppercase before processing.
- `ALL` returns sensor payload generated in sensors module.
- Other commands return echoed command plus local IP and may trigger side effects via `performSystemTask(...)`.

## Interrupt Handler

Function: `isr()`

Current behavior:
- Toggles built-in LED with blocking delays.
- Prints `in isr` to serial.

Important caution:
- Delay and serial operations inside ISR are generally unsafe on ESP platforms and can cause timing instability or watchdog resets under load.

## Module Dependencies

External functions used:
- `configSensors(char *sensorName)`
- `getSensorData(char *cmdFromClient, char *str)`
- `beginWIFI(String sensorName)`
- `performSystemTask(char *cmdFromClient)`

Libraries:
- Arduino core
- ESP8266WiFi

## Operational Notes

- Server and Wi-Fi startup are gated on at least one detected sensor.
- The current loop path for non-ALL commands writes to the client before and after task execution, but client is stopped before the trailing write path in that branch.
- Logging and timeout paths are synchronous and can add latency under heavy traffic.

## Suggested Hardening

- Move LED blink + serial print out of ISR; set a volatile flag in ISR and handle work in `loop()`.
- Unify response send/close flow to avoid writes after `client.stop()` paths.
- Add optional command allowlist/firewall check based on `client.remoteIP()`.
- Add explicit parsing for command tokens and argument validation.
- Consider non-blocking timeout handling to keep server responsive with frequent clients.
