# main.cpp

## Purpose
Implements the ESP8266 TCP server loop and command dispatch.

## Startup
- initializes serial
- calls `configSensors`
- starts server and WiFi only when at least one sensor is found
- attaches interrupt on `D6`

## Request Handling
- waits for TCP client on port `8888`
- uppercases incoming command bytes into `cmdFromClient`
- 5-second read timeout

Command paths:
- `ALL...` -> `getSensorData()` and return sensor response
- other -> return `<CMD>_<local_ip>`, then call `performSystemTask()`

## Side Effects
- prints client and response status to serial
- closes socket after each exchange

## Interrupt Note
`isr()` currently toggles LED with delays and prints serial output. This is functionally useful for testing but not ideal ISR design for production.
