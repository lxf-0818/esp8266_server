# setStatic.cpp

## Purpose
Legacy/experimental static IP assignment flow for ESP8266. Most code is currently commented out and not part of the active runtime path.

## Intended Flow (from commented implementation)
1. temporary WiFi connect using supplied SSID/password
2. query backend for MAC-based device index
3. derive static IP last octet from returned index
4. configure static IP with `WiFi.config(...)`
5. optionally update display/backend

## Key Functions in Commented Code
- `setStaticIP(sensorName, ssid, psw)`
- `tmpConnect(ssid, psw)`

## Current Status
- File is effectively inactive in build behavior due to commented content.
- Useful as a reference for previous provisioning strategy.

## Recommendation
If static IP mode is needed again, move this logic behind a compile-time flag and restore with explicit validation and timeout/error handling.
