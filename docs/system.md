# system.cpp

## Purpose
Provides command-triggered system actions for the ESP8266 server.

## API

### performSystemTask(cmdFromClient)
Parses incoming command text and executes a side effect.

Supported commands:
- contains `BLK`: drives `D6` low
- contains `RST`: calls `ESP.reset()`

## Behavior Notes
- Command matching uses substring checks (`strstr`), so partial matches trigger actions.
- `BLK` currently does not implement a full blink sequence in this module; it only writes the pin low.
- `RST` is immediate and non-graceful (hard reset behavior).
