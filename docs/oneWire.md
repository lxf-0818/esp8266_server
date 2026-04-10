# OneWire DS18B20 Module Documentation

This document describes the behavior of src/oneWire.cpp.

## Purpose

The module provides:
- DS18B20 device presence scan on the OneWire bus
- Per-device temperature conversion and reading
- Basic CRC validation for ROM and scratchpad
- CSV-like payload generation for upstream response assembly

## Hardware Binding

- OneWire data pin is fixed at GPIO4 (`D2` on ESP8266 dev boards).
- A single global OneWire instance is used:
  - `OneWire oneWire(oneWireBus);`

## Public API

### int scanOneWire()

Behavior:
- Iterates with `oneWire.search(addr)` until no more devices are found.
- Counts detected devices and resets search state when complete.

Returns:
- Number of devices discovered on the bus.

Notes:
- Does not validate ROM CRC during count.
- Intended as quick availability check.

### int readTemp(char *str)

Reads all devices currently found via search and builds formatted output.

Return codes:
- `0`: Success. Output string populated with device data.
- `1`: ROM CRC check failed (`OneWire::crc8(addr, 7) != addr[7]`).
- `2`: Scratchpad CRC check failed.
- `3`: No valid devices read (dropped devices / wiring issue).

Data acquisition flow per device:
1. `search(addr)` for next ROM.
2. Validate ROM CRC.
3. Issue temperature conversion command (`0x44`) with parasite power flag.
4. Delay 1000 ms for conversion.
5. Read scratchpad (`0xBE`) bytes.
6. Validate scratchpad CRC.
7. Convert raw value to Fahrenheit and store in local array.

Temperature conversion:
- Raw Celsius is computed as `raw / 16.0`.
- Fahrenheit conversion is `C * 1.8 + 32`.

## Output Format

When successful, output is appended to caller buffer in this shape:
- `<addr0_hex>,<temp_f>,|, ...`

Current implementation writes only `addr[0]` (first ROM byte) as device identifier.

On no-device path:
- Output set to `device(s) were dropped check wiring`
- Function returns `3`

On CRC failures:
- Output set to descriptive error text and corresponding code is returned.

## Debug Behavior

With `DEBUG` enabled:
- Prints device count in `scanOneWire()`.
- Prints ROM bytes when ROM CRC fails.
- Prints scratchpad-related debug data when scratchpad CRC fails.

## Operational Caveats

- Conversion delay is blocking (`delay(1000)`), which may reduce responsiveness in tight loops.
- Function supports up to 10 devices via local `temp[10]` buffer.
- Output formatting uses repeated `sprintf` without explicit destination-length checks.
- Search and read run against a global OneWire object; function is not re-entrant.

## Known Data Semantics

- `addr` buffer is reused each loop iteration.
- Final formatted rows use the currently held `addr[0]` as each row identifier.
- If stable per-device identity is needed, full ROM address persistence per reading should be stored before formatting.

## Suggested Hardening

- Add output buffer length parameter and switch to `snprintf`.
- Capture full 8-byte ROM per device in a struct and format deterministic IDs.
- Replace fixed delay with conversion-ready polling where timing budget allows.
- Return structured error metadata in addition to status code for diagnostics.
