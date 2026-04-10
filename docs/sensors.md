# sensors.cpp

## Purpose
Discovers available sensors on ESP8266, configures them, and builds telemetry payloads for socket responses.

## Supported Sensors
- BMP3XX (`0x77`) -> tag `BMX`
- BME280 (`0x76`) -> tag `BME`
- BMP280 (`0x76`/chip id) -> tag `BMP`
- SHT35 (`0x44`) -> tag `SHT`
- ADS1115 (`0x48`) -> tag `ADC`
- DS18B20 (one-wire) -> tag `DS1`

## Key APIs

### configSensors(sensorName)
- scans one-wire and I2C
- initializes available drivers
- appends discovered tags to `sensorName` with `_` separators
- returns number of installed sensors

### getSensorData(cmd, str)
- reads active sensors
- builds `,|,` separated record list
- computes CRC32
- encrypts payload unless `NO_SOCKET_AES` is defined
- returns string format:
  - encrypted mode: `<crc32_hex>:<ciphertext>`
  - no-aes mode: `<crc32_hex>:<plaintext>`

### scanI2Cports() / check_if_exist_I2C()
Brute-force scans SDA/SCL pin pair combinations and stores discovered device address + pin mapping.

### setWireBegin(addr)
Selects SDA/SCL pins associated with given I2C address and calls `Wire.begin(sda, scl)`.

## Payload Record Shape
Each sensor contributes one row:
`<sensor_id_or_addr>,<v1>,<v2>[,<v3>]`
Rows are concatenated using `,|,`.

## Constraints
- fixed-size buffers are used in several formatting paths
- scan is broad and can be slow/noisy in debug mode
- a discovered address map is required before per-sensor reads
