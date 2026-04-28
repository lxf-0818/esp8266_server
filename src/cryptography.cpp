/**
 * @file cryptography.cpp
 * @brief AES-CBC encryption and decryption helpers for the ESP8266 server.
 *
 * @details
 * Provides the low-level AES wrappers used by the rest of the firmware to
 * encrypt sensor payloads before transmission and to decrypt stored credentials
 * at startup.  All operations use the AESLib library with base-64 encoding so
 * that ciphertext can be safely carried in ASCII transports.
 *
 * Global buffers:
 * - `aes_key` / `aes_iv`      – active key and IV (N_BLOCK bytes each).
 * - `aes_iv_copy` / `aes_key_copy` – scratch copies consumed by each call
 *   (AESLib mutates both keys in-place, so originals are preserved this way).
 * - `cleartext`  – INPUT_BUFFER_LIMIT-byte plaintext workspace.
 * - `ciphertext` – 2×INPUT_BUFFER_LIMIT-byte base64-encoded output workspace.
 *
 * @note `new_aes_key` and `new_aes_iv` are reserved for future key-rotation
 *       support and are not currently used.
 *
 * @author Leon Freimour
 */
#include <Arduino.h>
#include <AESLib.h>
#include <LittleFS.h>
#define INPUT_BUFFER_LIMIT 2048


uint16_t encrypt_to_ciphertext(char *msg, byte iv[], byte key[]);
void encrypt_stub(char *str, char *str2);
void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[], byte key[], char *cleartext);
void aes_init();
int readAES(char *fileName, byte data[]);
String readLittle(char *fileName);
int readEncyptWifiCredentials(char *cssid_psw);


AESLib aesLib;
// AES Encryption Keys
byte aes_key[N_BLOCK];
byte aes_iv[N_BLOCK];
byte iv[N_BLOCK];

byte aes_iv_copy[N_BLOCK];
byte aes_key_copy[N_BLOCK];
char cleartext[INPUT_BUFFER_LIMIT] = {0};      // THIS IS INPUT BUFFER (FOR TEXT)
char ciphertext[2 * INPUT_BUFFER_LIMIT] = {0}; // THIS IS OUTPUT BUFFER (FOR BASE64-ENCODED ENCRYPTED DATA)


void aes_init()
{
  //aesLib.gen_iv(aes_iv);
  aesLib.set_paddingmode((paddingMode)0);
}

/**
 * @brief Encrypts a plaintext string using AES-CBC with a freshly generated IV.
 *
 * Generates a new random IV via `aesLib.gen_iv()`, then calls
 * `encrypt_to_ciphertext()` to perform the AES-CBC encryption.  The resulting
 * base64-encoded ciphertext is copied into `aes_encrypt`.  Working copies of
 * the IV and key (`aes_iv_copy`, `aes_key_copy`) are refreshed on every call
 * so the originals are not mutated by the AESLib internals.
 *
 * @param str         Null-terminated plaintext string to encrypt.
 * @param aes_encrypt Output buffer that receives the base64-encoded ciphertext.
 *                    Must be at least `2 * INPUT_BUFFER_LIMIT` bytes.
 */
void encrypt_stub(char *str, char *aes_encrypt)
{
  aesLib.gen_iv(aes_iv); 
  memcpy(aes_iv_copy, aes_iv, sizeof(aes_iv));
  memcpy(aes_key_copy, aes_key, sizeof(aes_key));
  int length = encrypt_to_ciphertext(str, aes_iv_copy, aes_key_copy);

  strncpy(aes_encrypt, ciphertext, length + 1);
  //Serial.printf("clear text      %s\n", str);
  //Serial.printf("encrypt string: %s\n", ciphertext);
}

/**
 * @brief Encrypts a plaintext message with AES-CBC and stores the result in the global `ciphertext` buffer.
 *
 * Computes the required base64 output length, encrypts `msg` using `aesLib.encrypt64()`,
 * then immediately performs a round-trip decrypt to verify correctness.  If the decrypted
 * output does not match the original plaintext, `"no match"` is printed to Serial and
 * the function returns `(uint16_t)-1`.
 *
 * @param msg  Null-terminated plaintext string to encrypt.
 * @param iv   16-byte AES IV.  Consumed (mutated) by AESLib; pass a copy, not the master IV.
 * @param key  16-byte AES key.  Consumed (mutated) by AESLib; pass a copy, not the master key.
 *
 * @return Length of the base64-encoded ciphertext written to the global `ciphertext` buffer,
 *         or `(uint16_t)-1` if the round-trip verification fails.
 *
 * @note The result is written to the module-global `ciphertext` buffer, not returned directly.
 *       Callers should read `ciphertext` after a successful return.
 */
uint16_t encrypt_to_ciphertext(char *msg, byte iv[], byte key[])
{
  int msgLen = strlen(msg);
  int cipherlength = aesLib.get_cipher64_length(msgLen);
  char encrypted_bytes[cipherlength];
  uint16_t enc_length = aesLib.encrypt64((byte *)msg, msgLen, encrypted_bytes, key, sizeof(aes_key), iv);
  sprintf(ciphertext, "%s", encrypted_bytes);

  // test aes en/de crypt to ensure we are good to go
  memcpy(aes_iv_copy, aes_iv, sizeof(aes_iv));
  memcpy(aes_key_copy, aes_key, sizeof(aes_key));

  decrypt_to_cleartext(ciphertext, strlen(ciphertext), aes_iv_copy, aes_key_copy, cleartext);

  if (strcmp(cleartext, msg)) {
    Serial.println("no match");
    enc_length = -1;
  }
  return enc_length;
}

/**
 * @brief Decrypts a base64-encoded AES-CBC ciphertext into plaintext.
 *
 * Calls `aesLib.decrypt64()` to decode and decrypt `msg`, then null-terminates
 * the result.  On ESP8266 builds, `ESP.getFreeHeap()` is called before
 * decryption as a lightweight heap diagnostic (result is discarded).
 *
 * @param msg        Base64-encoded ciphertext string to decrypt.
 * @param msgLen     Length of `msg` in bytes (excluding null terminator).
 * @param iv         16-byte AES IV.  Consumed (mutated) by AESLib; pass a copy.
 * @param key        16-byte AES key.  Consumed (mutated) by AESLib; pass a copy.
 * @param cleartext  Output buffer that receives the null-terminated plaintext.
 *                   Must be at least `INPUT_BUFFER_LIMIT` bytes.
 */
void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[], byte key[], char *cleartext)
{

#ifdef ESP8266
  // Serial.print("[decrypt_to_cleartext] free heap: ");
  ESP.getFreeHeap();
#endif
  uint16_t decLen = aesLib.decrypt64(msg, msgLen, (byte *)cleartext, key, sizeof(aes_key), iv);
  cleartext[decLen] = '\0'; // added lxf
}
/**
 * @brief Reads encrypted Wi-Fi credentials from a file in the LittleFS file system.
 *
 * This function attempts to mount the LittleFS file system and read the contents
 * of the file "/ssid_pass_aes.txt". The file is expected to contain encrypted Wi-Fi
 * credentials. The credentials are returned as a null-terminated C-style string
 * through the provided `ssid_psw` buffer.
 *
 * @param ssid_psw A pointer to a character array where the decrypted Wi-Fi credentials
 *                 will be stored. The array must be large enough to hold the credentials.
 *
 * @return int Returns 0 on success, or an error code on failure:
 *             - 1: Failed to mount the LittleFS file system.
 *             - 2: Failed to open the "/ssid_pass_aes.txt" file for reading.
 *
 * @note Ensure that the LittleFS file system is mounted and the credentials file exists.
 *       The caller is responsible for providing a sufficiently large buffer for `ssid_psw`.
 */


int readEncyptWifiCredentials(char *ssid_psw)
{
  String ssid_psw_aes;
  bool success = LittleFS.begin();
  if (!success)
  {
    Serial.println("Error mounting the file system");
    return 1;
  }
  ssid_psw_aes = readLittle((char *)"/ssid_pass_aes.txt");
  readAES((char *)"/aes.txt", aes_key);
  readAES((char *)"/iv.txt", iv); // read the "static" IV that was used to create encrypted ssid:pass
  aes_init();

  memcpy(aes_iv_copy, iv, sizeof(iv)); 
  memcpy(aes_key_copy, aes_key, sizeof(aes_key));
  decrypt_to_cleartext((char *)ssid_psw_aes.c_str(), ssid_psw_aes.length(), aes_iv_copy, aes_key_copy, cleartext);
  strcpy(ssid_psw, cleartext); // return ssid-pass  as *char
  return 0;
}

int readAES(char *fileName, byte data[])
{
  String tmp;
  File file = LittleFS.open(fileName, "r");
  if (!file)
  {
    Serial.printf("Failed to open %s file for reading\n", fileName);
    return 2;
  }
  tmp.clear();
  while (file.available())
    tmp.concat(static_cast<char>(file.read()));

  int foo, i = 0;
  char *token = strtok((char *)tmp.c_str(), ",");
  while (token != NULL)
  {
    sscanf(token, "%x", &foo); // convert ASCII string to hex 0xYY
    data[i++] = foo;
    token = strtok(NULL, ",");
  }
  file.close();
  return 0;
}
// Reads the full contents of `fileName` from LittleFS and returns them as a String.
// Returns an empty String and logs an error if the file cannot be opened.
String readLittle(char *fileName)
{
  String returnString;
  File file = LittleFS.open(fileName, "r");
  if (!file)
  {
    Serial.printf("Failed to open %s file for reading\n", fileName);
    return "";
  }
  returnString.clear();
  while (file.available())
    returnString.concat(static_cast<char>(file.read()));

  file.close();

  return returnString;
}

