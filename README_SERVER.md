# ESP8266 Server Documentation

## Overview
This project runs on an ESP8266 and provides a lightweight socket server for sensor nodes.
It listens for client commands on TCP port 8888, reads local sensor data, and returns payloads to an ESP32 client.

Key responsibilities:
- Discover and configure connected sensors (I2C and OneWire).
- Connect to Wi-Fi using encrypted credentials from LittleFS.
- Register its IP, MAC, and sensor set in a local HTTP database endpoint.
- Serve sensor snapshots and execute simple remote control commands.

Main runtime entry is in src/main.cpp.

## Build Environment
PlatformIO environment from platformio.ini:
- env name: esp12e
- platform: espressif8266
- board: esp12e
- framework: arduino
- monitor speed: 115200
- filesystem: littlefs
- build type: debug
- build flags: -Wall -Wextra

## Project Structure
- src/main.cpp: TCP server setup, request parsing, response handling.
- docs/main.md: Detailed setup/loop runtime flow, socket command handling, and ISR behavior reference.
- src/sensors.cpp: Sensor discovery, I2C scan, OneWire/DS18B20 support, payload assembly, CRC and optional AES.
- docs/sensors.md: Detailed sensor config, I2C scan routing, payload serialization, and CRC/AES flow reference.
- src/login.cpp: Wi-Fi startup, AES decrypt/encrypt helpers, OLED startup display, MySQL/PHP registration POST.
- docs/login.md: Detailed Wi-Fi bootstrap, AES credential decrypt flow, OLED status output, and DB registration reference.
- src/system.cpp: Remote command actions such as BLK and RST.
- src/oneWire.cpp: DS18B20 scan/read logic and formatted payload output.
- docs/oneWire.md: Detailed DS18B20 scan, CRC validation, payload format, and return-code reference.

## Runtime Flow
1. setup() starts Serial and calls configSensors(sensorName).
2. If at least one sensor is detected, setup() starts Wi-Fi server and beginWIFI(sensorName).
3. beginWIFI() decrypts SSID/password from LittleFS, connects Wi-Fi, optionally initializes SSD1306, and posts metadata to saveIP.php.
4. loop() accepts TCP clients and reads incoming command bytes.
5. If command contains ALL, getSensorData() returns current sensor snapshot payload.
6. Otherwise, server echoes command with local IP and runs performSystemTask().
7. Connection is closed after response is sent.

## Socket Command Protocol
Incoming commands are uppercased before processing.

Supported behavior:
- ALL: return full sensor payload from all configured sensors.
- BLK: handled by performSystemTask(), toggles D6 output low.
- RST: handled by performSystemTask(), resets the ESP8266.
- Any other command: echo as COMMAND_localIP.

Port:
- 8888 TCP

Timeout behavior:
- If client sends no data within about 5 seconds after connect, server closes connection.

## Sensor Discovery And Data Format
Sensor initialization is in src/sensors.cpp and includes:
- BMP3XX (BMX)
- BME280 (BME)
- BMP280 (BMP)
- SHT35 (SHT)
- ADS1115 (ADC)
- DS18B20 OneWire (DS1)

I2C strategy:
- Scans multiple SDA/SCL pin combinations from a predefined candidate list.
- Stores discovered device address and matching pin pair.
- Re-initializes Wire with matching pins before each sensor read.

Payload pipeline:
1. Build sensor CSV fragments.
2. Join fragments with separators.
3. Optionally AES-encrypt result (disabled if NO_SOCKET_AES is defined).
4. Compute CRC32.
5. Return as CRC_HEX:PAYLOAD.

## Wi-Fi And Credentials
Credential source:
- /ssid_pass_aes.txt in LittleFS (encrypted data).

Startup details:
- readEncyptWifiCredentials() loads encrypted credentials.
- decrypt_to_cleartext() recovers ssid:password.
- beginWIFI() applies 20-second connection timeout.

If decryption setup fails, firmware restarts.

## HTTP Registration Endpoint
On successful Wi-Fi connect, upDateDB() sends a POST to:
- http://192.168.1.252/saveIP.php

Posted fields include:
- api_key
- board=esp8266
- location
- IPv4Address
- macAddress
- sensor

## Filesystem Data
Expected files under data/:
- aes.txt
- iv.txt
- ssid_pass_aes.txt

Upload filesystem data before first boot if these files are missing.

## Common PlatformIO Commands
From esp8266_server root:

pio run
pio run -t upload
pio run -t uploadfs
pio device monitor

## Troubleshooting
- Build succeeds but upload fails: verify serial port, board selection, and cable quality.
- No sensors found: check wiring, power, and I2C pin compatibility.
- DS18B20 errors: inspect pull-up resistor and OneWire bus integrity.
- Client timeout in loop(): ensure client sends command bytes quickly after connect.
- Empty or invalid payload: verify sensor configuration flags and CRC behavior.

## Security Notes
- Avoid committing real Wi-Fi credentials, API keys, or encryption keys.
- Hardcoded local HTTP endpoint should be moved to external config.
- Consider authenticated transport between ESP32 client and ESP8266 server where feasible.

## Recommended Next Improvements
- Move hardcoded IP endpoints and API key into LittleFS config files.
- Add command authentication for socket requests before allowing RST/BLK actions.
- Add non-blocking handling in ISR-related logic and reduce use of delay in interrupt paths.
- Add parser and payload unit tests for CRC + encryption interoperability.
