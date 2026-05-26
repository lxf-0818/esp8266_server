# login.cpp

## Purpose
Handles the ESP8266 startup sequence to bring the server online:
- decrypt and apply stored WiFi credentials (delegated to `cryptography.cpp`)
- connect to WiFi with a 20-second timeout
- optionally initialise an SSD1306 OLED display over I2C
- purge any stale IP registration from the remote database
- register the board's MAC address, IP address, and sensor name

## Dependencies
- `cryptography.cpp` — `readEncyptWifiCredentials()` for credential decryption
- `system.cpp` — `setWireBegin()` for I2C bus initialisation

## Key APIs

### beginWIFI(sensorName)
Main entry point called from `setup()`.

1. Calls `readEncyptWifiCredentials()` to obtain the decrypted `SSID:PASSWORD` string; calls `ESP.restart()` on failure.
2. Splits on `:` to separate SSID and password, then calls `WiFi.begin()`.
3. Polls `WiFi.status()` every 500 ms for up to 20 seconds; returns `1` on timeout.
4. Probes I2C address `SSD_ADDR` (0x3C) via `Wire.beginTransmission()`; if an SSD1306 is present, initialises it and displays:
   - `"server PIO"`
   - local IP address
   - `sensorName`
5. Issues an HTTP GET to `deleteIP.php?key=<local_ip>` to purge any stale DB entry.
6. Calls `upDateTableIPstatic()` to POST the current MAC/IP/sensor record.

Return value:
- `0` success
- `1` WiFi connection timed out

### upDateTableIPstatic(sensorName)
HTTP POST to `http://192.168.1.252/saveIP.php` with form-encoded fields:

| Field | Value |
|---|---|
| `api_key` | hardcoded token |
| `board` | `esp8266` |
| `location` | `HOME` |
| `IPv4Address` | `WiFi.localIP()` |
| `macAddress` | `WiFi.macAddress()` |
| `sensor` | `sensorName` argument |

Logs the HTTP response code and payload to Serial. No return value.

### performHttpGet(url)
Thin HTTP GET wrapper.

- Returns the response body as an Arduino `String` on HTTP 200.
- Returns an empty `String` and logs the status code on any other response.
- Called by `beginWIFI()` to purge stale IP registrations on boot.

## Compile Flags
- `DEBUG_PHP` — enables verbose URL + response payload logging inside `performHttpGet()`.
