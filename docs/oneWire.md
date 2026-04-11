# oneWire.cpp

## Purpose
Implements DS18B20 one-wire discovery and temperature acquisition for the ESP8266 server.

## Hardware Mapping
- one-wire bus pin: GPIO4 (`D2`)

## APIs

### scanOneWire()
- scans for one-wire devices using ROM search
- returns the number of detected devices
- resets search state before exiting

### readTemp(str)
Reads temperature values from discovered DS18B20 devices and formats a payload string.

Return codes:
- `0`: success
- `1`: ROM CRC invalid
- `2`: scratchpad CRC invalid
- `3`: no devices found / devices dropped

## Data Flow
For each discovered sensor:
1. validates ROM CRC
2. starts conversion (`0x44`)
3. waits conversion delay
4. reads scratchpad (`0xBE`)
5. validates scratchpad CRC
6. converts raw reading to Fahrenheit

Output row style:
`<addr_byte0_hex>,<temp_f>,|,`

## Notes
- Uses blocking delay for conversion (`delay(1000)`), so readings are synchronous.
- Includes debug logging when `DEBUG` is defined.
- The payload final-character cleanup is manual and should be reviewed if format changes.
