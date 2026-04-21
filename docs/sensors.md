# sensors.cpp

## Purpose
Detects supported sensors, stores their bus mappings, and builds the socket telemetry payload.

## Supported Sensors and Tags
- BMP3XX at `0x77`: tag `BMX`.
- BME280 at `0x76`: tag `BME`.
- BMP280: at `0x58`:tag `BMP`.
- SHT35/SHT85 at`0x44`: tag `SHT`.
- ADS1115 at `0x48`: tag `ADC`.
- DS18B20 one-wire devices: at `0x28` : tag `DS1`.

## Key APIs

### configSensors(sensorName)
1. Calls `scanOneWire()` and adds one `DS1` tag per detected one-wire device.
2. Calls `scanI2Cports()` to discover I2C address-to-pin mappings.
3. Attempts sensor driver init in this order: BMP3XX, BME280, BMP280, SHT, ADS1115.
4. For ADS1115, sets `GAIN_ONE` (`+/-4.096V`, `0.125mV/bit`).
5. Builds `sensorName` as underscore-joined tags (for example `BME_ADC_DS1`).
6. Returns sensor count.

Sensor enable flags are persisted in `*_CNFG` globals and consumed by `getSensorData()`.

## Payload Assembly

### getSensorData(str)
- Reads each configured sensor and concatenates row strings separated by `,|,`.
- Removes final separator before checksum/encryption stage.
- If no devices are active, returns `no sensors found`.

Output format:
- AES enabled (default): `<crc32_hex>:<ciphertext>`.
- `NO_SOCKET_AES` build: `<crc32_hex>:<plaintext>`.

CRC is calculated over the post-encryption text (`tmp`) in AES mode, and over plaintext in `NO_SOCKET_AES` mode.

## Per-Sensor Row Formats
- BMP3XX: `77,<temp_f>,<pressure_hpa>,|,`
- BME280: `76,<temp_f>,<humidity>,<altitude>,|,`
- BMP280: `<BMP280_CHIPID>,<temp_f>,<altitude>,|,`
- SHT: `44,<temp_f>,<humidity>,|,`
- ADS1115: `48,<volts0>,<volts1>,<rRatio>|,`
- DS18B20: `<addr_byte0>,<temp_f>,|,` via `readTemp()`

## I2C Mapping Model

### scanI2Cports()
Brute-force scans all distinct SDA/SCL pairs from:
- labels: `D0..D7`
- pins: `16,5,4,0,2,14,12,13`

Each pair runs `check_if_exist_I2C()`.

### check_if_exist_I2C()
- probes addresses `1..126`.
- stores hit in `devices[]` as `{I2Caddr, sca, scl}`.
- on wire error code `4`, logs and resets with `ESP.reset()`.

### setWireBegin(addr)
Looks up `addr` in `devices[]`, runs `Wire.begin(sca, scl)`, returns `1` on success, `0` if not mapped.

## Compile Flags
- `DEBUG_SCAN`: enables verbose scan logs (enabled in current file).
- `NO_SOCKET_AES`: disables socket encryption path when uncommented.

## Constraints
- Uses fixed-size local buffers (`tmp[512]`, `encrypt_string[512]`).
- `DEVICES` is `5` for the I2C mapping array.
- Full SDA/SCL permutation scan is intentionally broad and can be slow/noisy.
