# login.cpp

## Purpose
Handles ESP8266 network startup and cryptography helpers:
- read encrypted WiFi credentials from LittleFS
- decrypt SSID/password
- connect to WiFi
- clean up stale IP registration via HTTP GET to `deleteIP.php`
- publish board metadata to local backend

## Filesystem Inputs
- `/ssid_pass_aes.txt` (required) — encrypted WiFi credential blob
- `/aes.txt` (required) — comma-separated hex bytes of the AES-128 key
- `/iv.txt` (required) — comma-separated hex bytes of the AES IV used to decrypt credentials

## Key APIs

### beginWIFI(sensorName)
1. calls `readEncyptWifiCredentials()` — on failure calls `ESP.restart()`.
2. loads key from `/aes.txt` and IV from `/iv.txt` via `readAES()`.
3. decrypts credential blob to `SSID:PASSWORD` (colon-separated).
4. joins WiFi with 20-second timeout.
5. probes `SSD_ADDR` (0x3C) over I2C; if present, initializes SSD1306 display (shows "server PIO", IP, sensor name).
6. calls `performHttpGet()` to delete stale IP entry: `deleteIP.php?key=<local_ip>`.
7. posts metadata to `saveIP.php` via `upDateDB()`.

Return value:
- `0` success
- `1` timeout/failure to join WiFi

### readEncyptWifiCredentials(ssid_psw)
Mounts LittleFS and reads `/ssid_pass_aes.txt` into provided buffer via `readLittle()`.
File-open failures inside `readLittle()` are silent (returns empty string); the caller receives an empty credential string rather than an error code.

Return value:
- `0` success
- `1` failed to mount LittleFS

### upDateDB(sensorName)
HTTP POST to `saveIP.php` with:
- api key
- board type (`esp8266`)
- location (`HOME`)
- IPv4 address
- MAC address
- sensor name list

### performHttpGet(url)
HTTP GET wrapper. Returns response string or empty string on non-200 status.
Called from `beginWIFI()` to purge stale IP registrations on boot.

## AES Helpers
- `aes_init()` — generates a new random IV (`aesLib.gen_iv`) and sets padding mode to `0`.
- `encrypt_to_ciphertext(msg, iv, key)` — AES-128 CBC encrypt + base64 encode into global `ciphertext[]`. Round-trip verifies by decrypting and comparing; returns `-1` (as `uint16_t`) on mismatch, otherwise returns ciphertext length.
- `encrypt_stub(str, aes_encrypt)` — generates a fresh IV, copies it, calls `encrypt_to_ciphertext`, then copies result into `aes_encrypt`.
- `decrypt_to_cleartext(msg, msgLen, iv, key, cleartext)` — AES-128 CBC decrypt from base64 into `cleartext` buffer; null-terminates result.

## LittleFS Helpers
- `readAES(fileName, data[])` — opens a comma-separated hex file (e.g. `a1,b2,...`) and parses it into a `byte[]` array.
- `readLittle(fileName)` — reads a full file from LittleFS and returns it as an Arduino `String`.

AES is configured with static 16-byte key (`aes_key`) in module scope.

## Compile Flags
- `DEBUG_PHP` — enables verbose logging of `performHttpGet()` URL and response payload.
