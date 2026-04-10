# Sensors Module Documentation

This document describes the behavior of src/sensors.cpp.

## Purpose

The module is responsible for:
- Detecting available sensors (I2C and OneWire)
- Initializing sensor drivers and tracking configuration state
- Collecting a sensor snapshot payload
- Optionally AES-encrypting payload data
- Prefixing payload with CRC32 for transport integrity

## Supported Sensors

I2C-based:
- BMP3XX at `0x77` (`BMX` tag)
- BME280 at `0x76` (`BME` tag)
- BMP280 at `0x76` with `BMP280_CHIPID` (`BMP` tag)
- SHT35 at `0x44` (`SHT` tag)
- ADS1115 at `0x48` (`ADC` tag)

OneWire:
- DS18B20 via `readTemp(...)` (`DS1` tag)

## Core Public Functions

### int configSensors(char *sensorName)

Behavior:
1. Scans OneWire devices via `scanOneWire()` and marks `DS1` entries.
2. Scans I2C pin permutations via `scanI2Cports()` and records discovered addresses/pins.
3. Attempts to initialize each known sensor driver.
4. Sets boolean configuration flags for successful inits.
5. Builds underscore-delimited sensor list into `sensorName`.

Return value:
- Number of installed/configured sensor entries.

Example sensorName output:
- `DS1_BMX_BME_ADC`

### void getSensorData(char *cmd, char *str)

Purpose:
- Builds current sensor payload and returns it as `CRC_HEX:PAYLOAD`.

Behavior:
1. Reads each configured sensor and appends formatted row fragments.
2. Removes trailing `,|,` separator.
3. Encrypts payload unless `NO_SOCKET_AES` is defined.
4. Computes CRC32 over transmitted payload bytes.
5. Writes final string into `str`.

Input:
- `cmd` currently unused.

Failure behavior:
- If no sensors are configured/available, writes `no sensors found`.

## Payload Format

Sensor rows use comma/pipe separators with hexadecimal sensor IDs:
- `<sensor_id_hex>,<value1>,<value2>,|,`

Final wire format:
- `<crc32_hex>:<payload_or_encrypted_payload>`

When encryption is enabled:
- CRC is computed on encrypted payload text.

## I2C Discovery and Routing

### void scanI2Cports()

Behavior:
- Tries all non-equal combinations of candidate SDA/SCL pins from `portArray`.
- Calls `check_if_exist_I2C()` for each pair.
- Optionally logs successful pin pair hits when `DEBUG_SCAN` is enabled.

### int check_if_exist_I2C()

Behavior:
- Scans addresses `0x01..0x7E` for ACK responses.
- Stores hits in global `devices[]` with associated current SDA/SCL pins.
- Returns count of detected devices for current pin pair scan.

Error behavior:
- On I2C error code `4`, prints message and resets MCU.

### int setWireBegin(int addr)

Behavior:
- Looks up an address in `devices[]` and applies matching `Wire.begin(sda, scl)`.
- Returns `1` when mapping exists, otherwise `0`.

## Global State and Flags

Sensor enabled flags:
- `BME_CNFG`, `BMP_CNFG`, `BMX_CNFG`, `ADC_CNFG`, `DS1_CNFG`, `SHT_CNFG`

I2C mapping state:
- `devices[DEVICES]`
- `myCnt` index for discovered devices

Port candidate lists:
- `portArray = {16, 5, 4, 0, 2, 14, 12, 13}`
- `portMap = {D0, D1, D2, D3, D4, D5, D6, D7}`

## Important Constants

- `DEVICES = 5`
- `SEALEVELPRESSURE_HPA = 1012.8`
- `SHT_ADDRESS = 0x44`
- `BMx_ADDRESS = 0x76`
- `BMPX_ADDRESS = 0x77`
- `ADC_ADDRESS = 0x48`

## Operational Notes

- Sensor reads are synchronous and may block on hardware access.
- `sensorName` construction assumes caller buffer is large enough.
- Address-to-pin mapping persistence depends on `devices[]` capacity and scan order.
- DS18B20 data is delegated to `readTemp(...)` in oneWire module.

## Known Risks

- `devices[]` has fixed capacity (`DEVICES=5`) while multi-port scans can discover more entries; this risks overflow of `myCnt` indexing.
- Multiple `sprintf` and `strcat` uses assume adequate destination buffer size.
- `cmd` parameter is unused in `getSensorData(...)`.
- Duplicate include of `Adafruit_BMP3XX.h` and `Adafruit_Sensor.h` is present (harmless but noisy).

## Suggested Hardening

- Guard `myCnt` writes to prevent `devices[]` overflow.
- Add explicit output buffer lengths and use `snprintf`/bounded concatenation.
- Reset or de-duplicate device mapping entries between scans.
- Move scan/measurement traces behind one consolidated debug macro.
- Consider storing structured payload fields before final serialization for safer formatting.
