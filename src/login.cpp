#include <Arduino.h>
#include <FS.h>
#include <time.h>
#include <string.h>
#include <ESP8266WiFi.h>
#include <AESLib.h>
#include <ESP8266mDNS.h>

#define PORT 8888
#define INPUT_BUFFER_LIMIT 2048
AESLib aesLib;

// AES Encryption Key (same as in node-js example)
byte aes_key[] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};
byte aes_iv[N_BLOCK] = {0x05, 0x18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte enc_iv_to[N_BLOCK] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte enc_iv_from[N_BLOCK] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char cleartext[INPUT_BUFFER_LIMIT] = {0};      // THIS IS INPUT BUFFER (FOR TEXT)
char ciphertext[2 * INPUT_BUFFER_LIMIT] = {0}; // THIS IS OUTPUT BUFFER (FOR BASE64-ENCODED ENCRYPTED DATA)

void aes_init();
int beginWIFI(String sensorName);
uint16_t encrypt_to_ciphertext(char *msg, byte iv[]);
void encrypt_stub(char *str, char *str2);
void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[], char *cleartext);
int readCiphertext(char *cssid_psw_aes);
int setStaticIP(String sensorName,char *ssid,char *psw);

int beginWIFI(String sensorName)
{
  String ssid, pass, temp;
  char cssid_psw_aes[580];
  int index;

  aes_init();
  if (readCiphertext(cssid_psw_aes)) // the WiFi credentials in file is encrypted 
    ESP.restart();
  memcpy(enc_iv_to, aes_iv, sizeof(aes_iv));
  decrypt_to_cleartext(cssid_psw_aes, strlen(cssid_psw_aes), enc_iv_to, cleartext);

  temp = cleartext;
  index = temp.indexOf(":");
  ssid = temp.substring(0, index);
  pass = temp.substring(index + 1);
  
  setStaticIP(sensorName,(char *)ssid.c_str(),(char *)pass.c_str());

  // Note: need to time out
  WiFi.begin(ssid.c_str(), pass.c_str()); // Connect to wifi
  // Wait for connection
  Serial.println("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid.c_str());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Port ");
  Serial.println(PORT); //
//  if (MDNS.begin("bedroom"))   //Start mDNS with name esp8266
//       Serial.println("MDNS started");
   return 0;
}
void aes_init()
{
  // aesLib.gen_iv(aes_iv);
  aesLib.set_paddingmode((paddingMode)0);
  memcpy(enc_iv_to, aes_iv, sizeof(aes_iv));
  memcpy(enc_iv_from, aes_iv, sizeof(aes_iv));
}

void encrypt_stub(char *str, char *aes_encrypt)
{

  memcpy(enc_iv_to, aes_iv, sizeof(aes_iv));
  encrypt_to_ciphertext(str, enc_iv_to);
  strcpy(aes_encrypt, ciphertext);
  Serial.printf("clear text      %s\n", str);
  Serial.printf("encrypt string: %s\n", ciphertext);
}
uint16_t encrypt_to_ciphertext(char *msg, byte iv[])
{
  int msgLen = strlen(msg);
  int cipherlength = aesLib.get_cipher64_length(msgLen);
  char encrypted_bytes[cipherlength];
  uint16_t enc_length = aesLib.encrypt64((byte *)msg, msgLen, encrypted_bytes, aes_key, sizeof(aes_key), iv);
  sprintf(ciphertext, "%s", encrypted_bytes);

  // test aes en/de crypt to ensure we good to go
  memcpy(enc_iv_to, aes_iv, sizeof(aes_iv));
  decrypt_to_cleartext(ciphertext, strlen(ciphertext), enc_iv_to, cleartext);
  // Serial.printf("decrypt str %s\n", cleartext);

  if (!strcmp(cleartext, msg))
    Serial.println("match");
  return enc_length;
}
void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[], char *cleartext)
{

#ifdef ESP8266
  // Serial.print("[decrypt_to_cleartext] free heap: ");
  ESP.getFreeHeap();
#endif
  uint16_t decLen = aesLib.decrypt64(msg, msgLen, (byte *)cleartext, aes_key, sizeof(aes_key), iv);
  cleartext[decLen] = '\0'; // added lxf
}
int readCiphertext(char *ssid_psw)
{
  String ssid_psw_aes;
  // Serial.println(decLen);

  bool success = SPIFFS.begin();
  if (!success)
  {
    Serial.println("Error mounting the file system");
    return 1;
  }
  // else
  // {
  //    Serial.println("File system mounted with success");
  //  }

  File file = SPIFFS.open("/ssid_pass_aes.txt", "r");
  if (!file)
  {
    Serial.println("Failed to open ssid_pass_aes.txt file for reading");
    return 2;
  }
  ssid_psw_aes.clear();
  while (file.available())
    ssid_psw_aes.concat(static_cast<char>(file.read()));

  file.close();
  strcpy(ssid_psw, ssid_psw_aes.c_str()); // return ssid-pass  as *char
  return 0;
}
