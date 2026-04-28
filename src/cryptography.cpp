#include <Arduino.h>
#include <AESLib.h>
#define INPUT_BUFFER_LIMIT 2048


uint16_t encrypt_to_ciphertext(char *msg, byte iv[], byte key[]);
void encrypt_stub(char *str, char *str2);
void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[], byte key[], char *cleartext);

AESLib aesLib;
// AES Encryption Keys
byte aes_key[N_BLOCK];
byte aes_iv[N_BLOCK];
byte new_aes_key[N_BLOCK];
byte new_aes_iv[N_BLOCK];
byte aes_iv_copy[N_BLOCK];
byte aes_key_copy[N_BLOCK];
char cleartext[INPUT_BUFFER_LIMIT] = {0};      // THIS IS INPUT BUFFER (FOR TEXT)
char ciphertext[2 * INPUT_BUFFER_LIMIT] = {0}; // THIS IS OUTPUT BUFFER (FOR BASE64-ENCODED ENCRYPTED DATA)


void aes_init()
{
  //aesLib.gen_iv(aes_iv);
  aesLib.set_paddingmode((paddingMode)0);
}

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
void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[], byte key[], char *cleartext)
{

#ifdef ESP8266
  // Serial.print("[decrypt_to_cleartext] free heap: ");
  ESP.getFreeHeap();
#endif
  uint16_t decLen = aesLib.decrypt64(msg, msgLen, (byte *)cleartext, key, sizeof(aes_key), iv);
  cleartext[decLen] = '\0'; // added lxf
}
