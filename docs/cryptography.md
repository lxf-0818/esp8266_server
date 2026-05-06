# cryptography.cpp

## Purpose
Provides AES-128-CBC encrypt/decrypt helpers and LittleFS I/O utilities used
across the firmware:
- initialise AES library (padding mode 0)
- encrypt plaintext to base64-encoded ciphertext
- decrypt base64-encoded ciphertext to plaintext
- read and parse comma-separated hex key/IV files from LittleFS
- mount LittleFS and decrypt stored WiFi credentials

## Global Buffers

| Symbol | Size | Role |
|---|---|---|
| `aes_key[N_BLOCK]` | 16 bytes | Active AES-128 key |
| `aes_iv[N_BLOCK]` | 16 bytes | Active AES IV (regenerated per encrypt call) |
| `iv[N_BLOCK]` | 16 bytes | Static IV loaded from `/iv.txt` (used for credential decryption) |
| `aes_iv_copy[N_BLOCK]` | 16 bytes | Scratch IV passed to AESLib (mutated in-place) |
| `aes_key_copy[N_BLOCK]` | 16 bytes | Scratch key passed to AESLib (mutated in-place) |
| `cleartext[]` | 2048 bytes | Plaintext workspace |
| `ciphertext[]` | 4096 bytes | Base64-encoded ciphertext output workspace |

> AESLib mutates both the key and IV in-place on every call. The `*_copy`
> buffers protect the originals by passing copies to each operation.

## Filesystem Inputs

- `/ssid_pass_aes.txt` — AES-CBC-encrypted, base64-encoded `SSID:PASSWORD` blob
- `/aes.txt` — comma-separated ASCII hex bytes of the 16-byte AES-128 key (e.g. `a1,b2,c3,...`)
- `/iv.txt` — comma-separated ASCII hex bytes of the 16-byte IV used only used for the credential blob was encrypted

## Key APIs

### aes_init()
Sets AESLib padding mode to `0` (zero-padding). Must be called before any
encrypt or decrypt operation. IV generation is **not** performed here; it is
done per-call inside `encrypt_stub()`.

### encrypt_stub(str, aes_encrypt)
High-level encrypt entry point.
1. Generates a fresh random IV via `aesLib.gen_iv()` into `aes_iv`.
2. Copies `aes_iv` → `aes_iv_copy` and `aes_key` → `aes_key_copy`.
3. Calls `encrypt_to_ciphertext()`.
4. Copies the result from the global `ciphertext[]` into `aes_encrypt`.

`aes_encrypt` must be at least `2 × INPUT_BUFFER_LIMIT` (4096) bytes.

### encrypt_to_ciphertext(msg, iv, key)
Low-level AES-128-CBC encrypt + base64 encode.
1. Calls `aesLib.encrypt64()` → result written to global `ciphertext[]`.
2. Performs a round-trip decrypt to verify correctness.
3. Returns ciphertext length on success, or `(uint16_t)-1` on verification mismatch (`"no match"` logged to Serial).

`iv` and `key` are consumed (mutated); pass copies, not the originals.

### decrypt_to_cleartext(msg, msgLen, iv, key, cleartext)
AES-128-CBC decrypt + base64 decode.
- Calls `aesLib.decrypt64()` and null-terminates the result in `cleartext`.
- On ESP8266 builds, `ESP.getFreeHeap()` is called as a heap diagnostic
  (result discarded; guarded by `#ifdef ESP8266`).

`iv` and `key` are consumed (mutated); pass copies.

### readEncyptWifiCredentials(ssid_psw)
Convenience wrapper that ties the filesystem and crypto layers together.
1. Mounts LittleFS; returns `1` on failure.
2. Reads `/ssid_pass_aes.txt` via `readLittle()`.
3. Loads key from `/aes.txt` and IV from `/iv.txt` via `readAES()`.
4. Calls `aes_init()`, then `decrypt_to_cleartext()`.
5. Copies the resulting `SSID:PASSWORD` string into `ssid_psw`.

Return value:
- `0` success
- `1` LittleFS mount failed

File-open failures inside `readLittle()` / `readAES()` yield empty/zero data
rather than a propagated error code.

## LittleFS Helpers

### readAES(fileName, data[])
Opens a comma-separated ASCII hex file (e.g. `a1,b2,c3,...`) and stores each
parsed byte into `data[]`.

Return value:
- `0` success
- `2` file could not be opened

### readLittle(fileName)
Reads the full contents of a LittleFS file and returns them as an Arduino
`String`. Returns an empty `String` and logs an error to Serial if the file
cannot be opened.

## Compile Flags

- `ESP8266` — enables the `ESP.getFreeHeap()` diagnostic call inside
  `decrypt_to_cleartext()`.
