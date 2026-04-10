# login.cpp

## Purpose
Handles ESP8266 network startup and cryptography helpers:
- read encrypted WiFi credentials from LittleFS
- decrypt SSID/password
- connect to WiFi
- publish board metadata to local backend

## Filesystem Inputs
- `/ssid_pass_aes.txt` (required)

## Key APIs

### beginWIFI(sensorName)
1. reads encrypted credential blob
2. decrypts to `SSID:PASSWORD`
3. joins WiFi with timeout
4. initializes optional SSD1306 display
5. posts metadata to local php endpoint

Return value:
- `0` success
- `1` timeout/failure to join WiFi

### readEncyptWifiCredentials(ssid_psw)
Mounts LittleFS and reads `/ssid_pass_aes.txt` into provided buffer.

### upDateDB(sensorName)
HTTP POST to `saveIP.php` with:
- api key
- board type
- location
- IPv4 address
- MAC address
- sensor name list

## AES Helpers
- `aes_init()`
- `encrypt_to_ciphertext()`
- `encrypt_stub()`
- `decrypt_to_cleartext()`

AES is configured with static key/IV arrays in code.
