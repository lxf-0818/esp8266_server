# system.cpp

## Purpose
Provides command-triggered system actions for the ESP8266 server.

## API

### performSystemTask(cmdFromClient)
Parses incoming command text and executes a side effect.

Supported commands:
- contains `BLK`: drives `D6` low (triggers `isr()` on `CHANGE` — see main.cpp for the LED blink sequence in the ISR attached to D6)
- contains `RST`: calls `ESP.reset()` — immediate hard reset

## Behavior Notes
- Command matching uses substring checks (`strstr`), so partial matches trigger actions.
- `BLK` only writes `D6` low in this module; the actual LED blink happens in `main.cpp`'s ISR which is attached to `D6` with `CHANGE` trigger.
- `RST` is immediate and non-graceful (hard reset, unsaved data lost).
