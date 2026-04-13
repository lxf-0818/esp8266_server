# main.cpp

## Purpose
Implements the ESP8266 TCP server loop and command dispatch.

## Startup
- initializes serial
- calls `configSensors`
- starts server and WiFi only when at least one sensor is found
- configures `D6` as `INPUT_PULLUP` and `LED_BUILTIN` as `OUTPUT`
- attaches ISR on `D6` (`CHANGE` trigger)

## Request Handling
- waits for TCP client on port `8888`
- uppercases incoming command bytes into `cmdFromClient`
- 5-second read timeout; discards data beyond buffer limit

Command paths:
- `ALL...` → `getSensorData()` fills `results`, then sent to client
- other → sends `<CMD>_<local_ip>` to client, closes socket, then calls `performSystemTask()`

**Note:** After the if/else block, `client.print(results)` and `client.stop()` are called again unconditionally. For the `ALL` path this is the intended send; for the non-`ALL` path the client is already stopped so the second call is a no-op.

## Side Effects
- prints client IP and response to serial
- closes socket after each exchange

## Interrupt Note
`isr()` (ICACHE_RAM_ATTR) toggles `LED_BUILTIN` once (HIGH 500 ms → LOW 500 ms) and prints to serial. Uses `delay()` inside ISR — functional for testing but not safe for production.
