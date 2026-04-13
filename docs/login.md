# login.cpp

## Purpose
Handles ESP8266 network startup and cryptography helpers:
- read encrypted WiFi credentials from LittleFS
- decrypt SSID/password
- connect to WiFi
- clean up stale IP registration via HTTP GET to `deleteIP.php`
- publish board metadata to local backend

## Filesystem Inputs
- `/ssid_pass_aes.txt` (required)

## Key APIs

### beginWIFI(sensorName)
1. reads encrypted credential blob via `readEncyptWifiCredentials()`
2. decrypts to `SSID:PASSWORD` (colon-separated)
3. joins WiFi with 20-second timeout
4. initializes optional SSD1306 display (shows "server PIO", IP, sensor name)
5. calls `performHttpGet()` to delete stale IP entry: `deleteIP.php?key=<local_ip>`
6. posts metadata to `saveIP.php` via `upDateDB()`

Return value:
- `0` success
- `1` timeout/failure to join WiFi

### readEncyptWifiCredentials(ssid_psw)
Mounts LittleFS and reads `/ssid_pass_aes.txt` into provided buffer.

Return value:
- `0` success
- `1` failed to mount LittleFS
- `2` failed to open file

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
- `aes_init()` — sets padding mode
- `encrypt_to_ciphertext(msg, iv)` — AES-128 encrypt + base64, also round-trip verifies by decrypting and comparing
- `encrypt_stub(str, aes_encrypt)` — convenience wrapper: copies IV, encrypts, copies result
- `decrypt_to_cleartext(msg, msgLen, iv, cleartext)` — AES-128 decrypt from base64

AES is configured with static 16-byte key/IV arrays in code.
