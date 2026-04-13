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
3. waits conversion delay (`delay(1000)`)
4. reads scratchpad (`0xBE`, 9 bytes)
5. validates scratchpad CRC
6. converts raw 16-bit reading to Fahrenheit: `(raw / 16.0) * 1.8 + 32`

Output row style:
`<addr_byte0_hex>,<temp_f>,|,`

**Known issue:** The `addr[8]` array is overwritten on each loop iteration. After the search loop, `addr[0]` holds only the **last** device's address byte. All output rows therefore share the same address prefix — multi-device identification is unreliable.

After formatting, the last character of the output string is overwritten with `'0'` to remove the trailing comma.

## Notes
- Uses blocking delay for conversion (`delay(1000)`), so readings are synchronous.
- Supports up to 10 devices (`temp[10]` array).
- Includes debug logging when `DEBUG` is defined.
- The payload final-character cleanup is manual and should be reviewed if format changes.
