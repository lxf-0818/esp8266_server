# main.cpp

## Purpose
Runs the ESP8266 TCP server on port `8888` and dispatches incoming commands to either
sensor reads or system tasks.

## Setup Sequence
`setup()` performs the following in order:
1. Starts serial at `115200`.
2. Calls `configSensors(sensorName)`.
3. Starts `server.begin()` and `beginWIFI(sensorName)` only if at least one sensor is detected.
4. If no sensor is found, logs `No Device Found check wiring` and skips WiFi/server startup.
5. Configures `LED_BUILTIN` as `OUTPUT` and `D6` as `INPUT_PULLUP`.
6. Attaches ISR on `D6` using `CHANGE` trigger.
5. Configures `LED_BUILTIN` as `OUTPUT` and `D6` as `INPUT_PULLUP`.
6. Attaches ISR on `D6` using `CHANGE` trigger.

## Request Handling
- Accepts a client with `server.accept()`.
- Logs remote IP once connected.
- Waits up to `5000 ms` for input bytes, then times out and closes the socket.
- Uppercases each received byte into `cmdFromClient[80]`.
- Discards bytes beyond `79` characters to avoid command-buffer overflow.

## Command Routing
- `ALL` prefix: calls `getSensorData(cmdFromClient, results)`.
- Any other command:
	- builds `<CMD>_<local_ip>` into `results`.
	- calls `performSystemTask(cmdFromClient)`.

After routing, the response is sent once with `client.print(results)`, then the connection is closed with `client.stop()`.

## Buffers and Limits
- `cmdFromClient[80]`: command input buffer.
- `results[512]`: response payload buffer for both sensor and control paths.
- `sensorName[100]`, `str[80]`, `Buf[80]`: startup/local temporary buffers.

## Side Effects
- Prints client details and response payload to serial output.
- Closes the client socket after every handled request.

## ISR Note
`isr()` is marked `ICACHE_RAM_ATTR` and toggles `LED_BUILTIN` once using two `delay(500)` calls,
then prints `in isr` to serial. This is acceptable for bring-up/testing, but `delay()` and serial I/O
inside an ISR are unsafe for production firmware.
