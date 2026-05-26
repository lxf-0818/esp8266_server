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

## I2C Mapping Model

### scanI2Cports()
Brute-force scans all distinct SDA/SCL pairs from:
- labels: `D0..D7`
- pins: `16,5,4,0,2,14,12,13`

Note: the scan is intentionally broad so the caller does not need to know the exact SDA/SCL pairing in advance.

Each pair runs `check_if_exist_I2C()`.

### check_if_exist_I2C()
- probes only the known supported sensor addresses: `0x44, 0x76, 0x18, 0x58, 0x48, 0x77` (targeted address check, not a full bus scan).
- stores each hit in `devices[]` as `{I2Caddr, sca, scl}` using the active SDA/SCL pair from the surrounding scan loop.
- appends into `devices[myCnt]`, so the caller must keep `myCnt` consistent with the scan workflow.
- on Wire error code `4`, logs and resets with `ESP.reset()`.

### setWireBegin(addr)
Looks up `addr` in the populated `devices[]` table, runs `Wire.begin(sca, scl)`, returns `1` on success, `0` if not mapped.

Sensor enable flags are persisted in `*_CNFG` globals and consumed by `getSensorData()`.

## Payload Assembly

### getSensorData(cmd, str)
- `cmd` is unused (silenced with `(void)cmd`).
- Reads each configured sensor and concatenates row strings separated by `,|,`.
- Removes final separator before checksum/encryption stage.
- If no devices are active, writes `no sensors found` to `str` and returns early.
- If SHT is configured but `sht.dataReady()` is false, writes `SHT data not ready` to `str` and skips SHT data.

Output format:
- AES enabled (`SOCKET_AES` defined by default): `<crc32_hex>:<ciphertext>:<iv_hex_csv>`.
  - IV is appended as 16 comma-separated two-digit hex bytes (e.g. `a1,b2,...,ff`).
- AES disabled build: `<crc32_hex>:<plaintext>`.


## Per-Sensor Row Formats
- BMP3XX: `77,<temp_f>,<pressure_hpa>,|,`
- BME280: `76,<temp_f>,<humidity>,<altitude>,|,`
- BMP280: `<BMP280_CHIPID>,<temp_f>,<altitude>,|,`
- SHT: `44,<temp_f>,<humidity>,|,`
- ADS1115: `48,<volts0>,<volts1>,<rRatio>|,`
- DS18B20: `<addr_byte0>,<temp_f>,|,` via `readTemp()`


## Compile Flags
- `DEBUG_SCAN`: enables verbose scan logs; currently disabled by default.
- `SOCKET_AES`: enables AES encryption of the socket payload; currently enabled by default.

## Helper Functions

### convert2hexAscii(iv)
Converts a 16-byte AES IV array to a comma-separated two-digit hex ASCII string (e.g. `a1,b2,…,ff`). The trailing comma is stripped. Returns an Arduino `String`.

### printDevice(deviceNo)
Prints the I2C address, SDA pin, and SCL pin of a `devices[]` entry to Serial. Debug utility.

## Constraints
- Uses fixed-size local buffers (`tmp[512]`, `encrypt_string[512]`).
- `DEVICES` is `6` for the I2C mapping array.
- Full SDA/SCL permutation scan is intentionally broad and can be noisy when `DEBUG_SCAN` is defined.
