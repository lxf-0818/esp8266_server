# Login, Wi-Fi, and Crypto Module Documentation

This document describes the behavior of src/login.cpp.

## Purpose

The module provides:
- Wi-Fi credential loading from LittleFS
- AES-based decrypt/encrypt helpers
- Wi-Fi connection bootstrap with timeout
- OLED status display for server identity
- HTTP POST registration of board metadata to backend

## Key Responsibilities

- Read encrypted `ssid:password` payload from filesystem file.
- Decrypt credentials using AESLib and static key/IV configuration.
- Connect ESP8266 to Wi-Fi with bounded retry time.
- Display runtime info (`server PIO`, IP, sensorName) on SSD1306 when available.
- Post IP/MAC/sensor inventory to `saveIP.php`.

## Constants and Globals

- `SCREEN_WIDTH = 128`
- `SCREEN_HEIGHT = 64`
- `SSD_ADDR = 0x3c`
- `PORT = 8888`
- `INPUT_BUFFER_LIMIT = 2048`

Crypto globals:
- `aes_key[16]` (static key bytes)
- `aes_iv[N_BLOCK]`
- `enc_iv_to[N_BLOCK]`, `enc_iv_from[N_BLOCK]`
- `cleartext[INPUT_BUFFER_LIMIT]`
- `ciphertext[2 * INPUT_BUFFER_LIMIT]`

Display:
- `Adafruit_SSD1306 display(...)`

## Public Functions

### int beginWIFI(String sensorName)

Behavior:
1. Reads encrypted credential blob from LittleFS via `readEncyptWifiCredentials(...)`.
2. Initializes AES settings (`aes_init()`).
3. Copies IV and decrypts credentials into `cleartext`.
4. Splits decrypted text on `:` into SSID and password.
5. Calls `WiFi.begin(...)` and waits up to 20 seconds.
6. If connected, probes SSD1306 and renders:
   - server PIO
   - local IP
   - `sensorName`
7. Logs connection metadata and port.
8. Calls `upDateDB(sensorName)` to register endpoint data.

Return values:
- `0`: success
- `1`: Wi-Fi connection timeout

Failure behavior:
- If encrypted credential read fails, calls `ESP.restart()`.

### void aes_init()

Behavior:
- Configures AESLib padding mode to `0`.

### uint16_t encrypt_to_ciphertext(char *msg, byte iv[])

Behavior:
- Encrypts plaintext to Base64 ciphertext.
- Stores ciphertext in global `ciphertext` buffer.
- Performs round-trip decrypt check and prints `match` on equality.

Returns:
- encrypted output length

### void encrypt_stub(char *str, char *aes_encrypt)

Behavior:
- Resets IV copy and encrypts `str`.
- Copies encrypted output to caller buffer.
- Prints plaintext and ciphertext to serial.

### void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[], char *cleartext)

Behavior:
- AES-decrypts Base64 input into cleartext buffer.
- Appends null terminator using returned decrypt length.

### int readEncyptWifiCredentials(char *ssid_psw)

Behavior:
- Mounts LittleFS.
- Reads `/ssid_pass_aes.txt` into provided output buffer.

Returns:
- `0` success
- `1` LittleFS mount failure
- `2` file open failure

### void upDateDB(String sensorName)

Behavior:
- Collects MAC and local IP.
- Builds form-encoded payload with static fields (`api_key`, board, location).
- Sends HTTP POST to `http://192.168.1.252/saveIP.php`.
- Logs HTTP status code and response payload.

## Filesystem Contract

Required file:
- `/ssid_pass_aes.txt`

Expected decrypted payload format:
- `<ssid>:<password>`

## Runtime Data Contract

`beginWIFI(sensorName)` expects:
- `sensorName` already populated by sensor configuration layer.
- Valid AES key/IV values matching credential encryption format.

## Security and Reliability Notes

- AES key and API key are hardcoded in firmware source.
- HTTP registration endpoint is plain HTTP and fixed IP.
- Debug prints include sensitive plaintext/ciphertext in encryption path.
- Wi-Fi credentials parsing assumes a valid `:` separator is present.

## Suggested Hardening

- Move AES key/IV and API key to secured configuration storage.
- Guard against missing `:` during SSID/password split.
- Avoid serial logging of sensitive plaintext/ciphertext in production.
- Add retry/backoff policy for registration POST failures.
- Consider HTTPS and endpoint authentication for `saveIP.php`.
