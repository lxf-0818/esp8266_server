# sensors.cpp

## Purpose
Discovers available sensors on ESP8266, configures them, and builds telemetry payloads for socket responses.

## Supported Sensors
- BMP3XX (`0x77`) → tag `BMX`
- BME280 (`0x76`) → tag `BME`
- BMP280 (`0x76` / chip id `BMP280_CHIPID`) → tag `BMP`
- SHT85 (`0x44`) → tag `SHT` (uses `SHT85.h` library, `SHT35` class)
- ADS1115 (`0x48`) → tag `ADC`
- DS18B20 (one-wire) → tag `DS1`

## Key APIs

### configSensors(sensorName)
- scans one-wire first, then I2C via `scanI2Cports()`
- initializes available drivers (BMP3XX, BME280, BMP280, SHT, ADS1115)
- ADS1115 gain set to `GAIN_ONE` (+/- 4.096 V, 0.125 mV resolution)
- appends discovered tags to `sensorName` with `_` separators
- returns number of installed sensors

### getSensorData(cmd, str)
- reads active sensors (if `*_CNFG` flags are set)
- builds `,|,` separated record list (trailing `,|,` stripped before output)
- computes CRC32
- encrypts payload unless `NO_SOCKET_AES` is defined
- returns string format:
  - encrypted mode: `<crc32_hex>:<ciphertext>`
  - no-aes mode: `<crc32_hex>:<plaintext>`

### scanI2Cports() / check_if_exist_I2C()
Brute-force scans SDA/SCL pin pair combinations (D0–D7) and stores discovered device address + pin mapping in `devices[]` array. Resets ESP on unknown I2C error (error code 4).

### setWireBegin(addr)
Selects SDA/SCL pins associated with given I2C address and calls `Wire.begin(sda, scl)`.
- Returns `1` if address found in `devices[]` and Wire initialized
- Returns `0` if address not found (Wire not re-initialized)

## Payload Record Shape
Each sensor contributes one row. Format varies by sensor:

| Sensor | Format | Fields |
|--------|--------|--------|
| BMP3XX | `77,<temp_f>,<pressure_hpa>,\|,` | temp (°F), pressure (hPa) |
| BME280 | `76,<temp_f>,<humidity>,<altitude>,\|,` | temp (°F), humidity (%), altitude (m) |
| BMP280 | `<BMP280_CHIPID>,<temp_f>,<altitude>,\|,` | temp (°F), altitude (m) — uses chip ID not I2C addr |
| SHT85 | `44,<temp_f>,<humidity>,\|,` | temp (°F), humidity (%) |
| ADS1115 | `48,<volts0>,<volts1>,<rRatio>\|,` | A0 volts, A1 volts (ESP 3V ref), resistor divider ratio (5.63 = (220k+47k)/47k) |
| DS18B20 | `<addr_byte0>,<temp_f>,\|,` | per-device via `readTemp()` |

Rows are concatenated using `,|,`.

## Constraints
- fixed-size buffers (512 bytes) used in formatting paths
- I2C scan is broad across all D0–D7 pin pairs; can be slow/noisy in debug mode
- a discovered address map (`devices[]`) is required before per-sensor reads
