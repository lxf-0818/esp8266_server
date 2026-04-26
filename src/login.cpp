/**
 * @file login.cpp
 * @brief This file contains the implementation of WiFi initialization, AES encryption/decryption,
 *        and database update functionalities for an ESP8266-based server project.
 *
 * @details
 * - The code initializes WiFi credentials stored in an AES-encrypted file using LittleFS.
 * - It handles AES encryption and decryption for secure storage and transmission of sensitive data.
 * - It updates a remote database with the device's IP address and MAC address.
 * - It also configures an OLED display to show connection details.
 *
 * @libraries
 * - Arduino.h: Core Arduino functions.
 * - FS.h: File system support.
 * - time.h: Time-related functions.*
 * - ESP8266WiFi.h: WiFi functionality for ESP8266.
 * - AESLib.h: AES encryption library.
 * - LittleFS.h: LittleFS file system support.
 * - ESP8266HTTPClient.h: HTTP client for ESP8266.
 * - Adafruit_SSD1306.h: OLED display library.
 *
 * @defines
 * - SCREEN_WIDTH: Width of the OLED display in pixels.
 * - SCREEN_HEIGHT: Height of the OLED display in pixels.
 * - SSD_ADDR: I2C address of the OLED display.
 * - PORT: Port number for the server.
 * - INPUT_BUFFER_LIMIT: Maximum size of input buffer for encryption/decryption.
 *
 * @functions
 * - int beginWIFI(String sensorName): Initializes WiFi connection using AES-decrypted credentials.
 * - void aes_init(): Initializes AES encryption settings.
 * - uint16_t encrypt_to_ciphertext(char *msg, byte iv[]): Encrypts a message using AES and returns the ciphertext length.
 * - void encrypt_stub(char *str, char *aes_encrypt): Encrypts a string and stores the result in a buffer.
 * - void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[], char *cleartext): Decrypts a ciphertext into cleartext.
 * - int readEncyptWifiCredentials(char *cssid_psw_aes): Decrypts WiFi credentials stored in a file.
 * - void upDateDB(String sensorName): Updates a remote database with device information.
 *
 * @notes
 * - The WiFi credentials are stored in an AES-encrypted file named "ssid_pass_aes.txt" on the LittleFS file system.
 * - The OLED display is used to show the server name, IP address, and sensor name after successful WiFi connection.
 * - The database update function sends a POST request to a remote server with device details.
 *
 * @author Leon Freimour
 * @date 2025-3-28
 *
 * @dependencies
 */
#include <Arduino.h>
#include <FS.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <AESLib.h>
#include <LittleFS.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define SSD_ADDR 0x3c
#define PORT 8888
#define INPUT_BUFFER_LIMIT 2048
AESLib aesLib;
// AES Encryption Keys
byte aes_key[N_BLOCK];
byte aes_iv[N_BLOCK];
byte new_aes_key[N_BLOCK];
byte new_aes_iv[N_BLOCK];
byte aes_iv_copy[N_BLOCK];
char cleartext[INPUT_BUFFER_LIMIT] = {0};      // THIS IS INPUT BUFFER (FOR TEXT)
char ciphertext[2 * INPUT_BUFFER_LIMIT] = {0}; // THIS IS OUTPUT BUFFER (FOR BASE64-ENCODED ENCRYPTED DATA)

int setWireBegin(int addr);
void aes_init();
int readAES(char *fileName, byte data[]);
String readLittle(char *fileName);
int beginWIFI(String sensorName);
uint16_t encrypt_to_ciphertext(char *msg, byte iv[], byte key[]);
void encrypt_stub(char *str, char *str2);
void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[], byte key[], char *cleartext);
int readEncyptWifiCredentials(char *cssid_psw_aes);
void upDateDB(String sensorName);
String performHttpGet(const char *url);
void conver2hexAscii(unsigned char *iv);
int writeLittle(char *fileName, const char *message);

/**
 * @brief Initializes the Wi-Fi connection and sets up the display and database.
 *
 * Function "beginWiFi()" retrieves Wi-Fi credentials stored in an AES-encrypted file on the chip,
 * decrypts them, and attempts to connect to the specified Wi-Fi network. If the connection
 * is successful, it initializes the SSD1306 display if connected to show the server information and updates
 * the database with the provided sensor name.
 *
 * @param sensorName A string representing the name of the sensor to be displayed and updated in the database.
 * @return int Returns 0 on success, or 1 if the Wi-Fi connection fails within the timeout period.
 *
 * @note The Wi-Fi credentials are stored in a text file on the chip using LittleFS and are AES-encrypted.
 *       The function will restart the ESP device if decryption of Wi-Fi credentials fails.
 *
 * @details
 * - The function uses a timeout of 5 seconds to attempt a Wi-Fi connection.
 * - If the Wi-Fi connection is successful, the SSD1306 display is initialized to show:
 *   - "server PIO"
 *   - The local IP address of the device
 *   - The provided sensor name
 * - The function also updates the database with the sensor name.
 * - If the SSD1306 display initialization fails, an error message is printed to the serial monitor.
 *
 * @dependencies
 * - Requires the AES encryption/decryption functions (`aes_init`, `readEncyptWifiCredentials`, `decrypt_to_cleartext`).
 * - Requires the Wi-Fi library for ESP8266 (`WiFi`).
 * - Requires the SSD1306 display library (`Adafruit_SSD1306`).
 * - Requires the Wire library for I2C communication.
 */
int beginWIFI(String sensorName)
{
  String ssid, pass, temp;
  char cssid_psw_aes[580];
  int index;
  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 20000; // 20 seconds timeout

  if (readEncyptWifiCredentials(cssid_psw_aes))
    ESP.restart();
  readAES((char *)"/aes.txt", aes_key);
  readAES((char *)"/iv.txt", aes_iv);
  aes_init();
 // conver2hexAscii(aes_iv);
  // printf("Hex String: %s\n", foo.c_str());

  // WARNING: make a copy the below function will corrutped the input byte array
  memcpy(aes_iv_copy, aes_iv, sizeof(aes_iv));
  decrypt_to_cleartext(cssid_psw_aes, strlen(cssid_psw_aes), aes_iv_copy, aes_key, cleartext);

  temp = cleartext;
  index = temp.indexOf(":"); // get eos token
  ssid = temp.substring(0, index);
  pass = temp.substring(index + 1);

  // Note: need to time out
  WiFi.begin(ssid.c_str(), pass.c_str()); // Connect to wifi

  Serial.println("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout)
  {
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Failed to connect to Wi-Fi within timeout.");
    return 1; // Return an error code
  }

  setWireBegin(SSD_ADDR);
  Wire.beginTransmission(SSD_ADDR);
  bool SSD_CONFIG = Wire.endTransmission();
  if (!SSD_CONFIG)
  {
    if (!display.begin(SSD1306_SWITCHCAPVCC, SSD_ADDR)) // Address 0x3D for 128x64
      Serial.println(F("SSD1306 allocation failed"));
    else
    {
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.println("server PIO");
      display.println(WiFi.localIP());
      display.println(sensorName);
      display.display();
    }
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid.c_str());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Port ");
  Serial.println(PORT);

  // remove stale entry in DB based on IP@
  String IP = WiFi.localIP().toString();
  String phpScript = "http://192.168.1.252/deleteIP.php?key=" + (String)IP;
  performHttpGet(phpScript.c_str());

  upDateDB(sensorName); // only single sensor on

  return 0;
}
/**
 * @brief Initializes the AES encryption library with the desired settings.
 *
 * This function sets up the AES library by configuring the padding mode.
 * The initialization vector (IV) generation is currently commented out.
 *
 * Note:
 * - Ensure that the AES library is properly included and initialized before calling this function.
 * - The padding mode is set using the `set_paddingmode` method with a specific mode.
 */
void aes_init()
{
  // aesLib.gen_iv(aes_iv);
  aesLib.set_paddingmode((paddingMode)0);
}

void encrypt_stub(char *str, char *aes_encrypt)
{

  memcpy(aes_iv_copy, aes_iv, sizeof(aes_iv));
  int length = encrypt_to_ciphertext(str, aes_iv_copy, aes_key);

  strncpy(aes_encrypt, ciphertext, length + 1);
  Serial.printf("clear text      %s\n", str);
  Serial.printf("encrypt string: %s\n", ciphertext);
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
  decrypt_to_cleartext(ciphertext, strlen(ciphertext), aes_iv_copy,key, cleartext);
  Serial.printf("decrypt str %s\n", cleartext);

  if (!strcmp(cleartext, msg)) // need mod == 0
    Serial.println("match");
  else
    Serial.println("no match");
  return enc_length;
}
void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[],byte key[], char *cleartext)
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
 * @note Ensure that the LittleFS library is properly initialized in your project. // wtf?
 *       The caller is responsible for providing a sufficiently large buffer for `ssid_psw`.
 */
int readEncyptWifiCredentials(char *ssid_psw)
{
  String ssid_psw_aes,iv_ssid_psw_aes;
  // Serial.println(decLen);

  bool success = LittleFS.begin();
  if (!success)
  {
    Serial.println("Error mounting the file system");
    return 1;
  }
  ssid_psw_aes = readLittle((char *)"/ssid_pass_aes.txt");
  // no reason to do this
  // iv_ssid_psw_aes = readLittle((char *)"/ssid_pass_aes_copy.txt");
  // unsigned int foo;
  // byte tmp_iv[16];
  // int i = 0;
  // char *token = strtok((char *)iv_ssid_psw_aes.c_str(), ",");
  // while (token != NULL)
  // {
  //   sscanf(token, "%x", &foo); // convert ASCII string to hex 0xYY
  //   tmp_iv[i++] = foo;
  //   token = strtok(NULL, ",");
  // }

  strcpy(ssid_psw, ssid_psw_aes.c_str()); // return ssid-pass  as *char
  return 0;
}
/**
 * @brief Sends an HTTP POST request to update the database with sensor and device information.
 *
 * This function collects device information such as MAC address, IP address, and sensor name,
 * and sends it to mySQL via HTTP POST request. The PHP server is expected
 * to handle the data and update the database accordingly.
 *
 * @param sensorName The name of the sensor to be included in the HTTP request.
 *
 * @note The function uses the ESP8266 WiFi library and HTTPClient for network communication.
 *       Ensure that the device is connected to WiFi before calling this function.
 *
 * @details
 * - The server endpoint is hardcoded as "http://192.168.1.252/saveIP.php".
 * - The API key, board type, and location are also hardcoded within the function.
 * - The function logs the HTTP response code and payload to the serial monitor.
 *
 * @return void
 */
void upDateDB(String sensorName)
{
  WiFiClient client_sql;
  HTTPClient http;
  char macAddr[80];
  String payload, IP, httpRequestData, serverName, apiKeyValue, sensorLocation;

  WiFi.macAddress().toCharArray(macAddr, sizeof(macAddr));
  IP = WiFi.localIP().toString();
  apiKeyValue = "tPmAT5Ab3j7F9", sensorLocation = "HOME";

  httpRequestData = "api_key=" + apiKeyValue;
  httpRequestData += "&board=esp8266";
  httpRequestData += "&location=" + sensorLocation;
  httpRequestData += "&IPv4Address=" + IP;
  httpRequestData += "&macAddress=" + (String)macAddr;
  httpRequestData += "&sensor=" + sensorName;
  serverName = "http://192.168.1.252/saveIP.php";
  http.begin(client_sql, serverName.c_str());
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  // delay(500);
  int httpResponseCode = http.POST(httpRequestData);
  payload = http.getString();
  Serial.printf("http rc %d payload %s \n\t %s\n", httpResponseCode, payload.c_str(), httpRequestData.c_str());
  http.end();
  return;
}
String performHttpGet(const char *url)
{
  WiFiClient client_sql;
  HTTPClient http;
  http.begin(client_sql, url);
  int httpResponseCode = http.GET();
  if (httpResponseCode != 200)
  {
    Serial.printf("HTTP GET failed with code: %d\n", httpResponseCode);
    return ""; // Return an empty string on failure
  }
  String response = http.getString();
  http.end();

#ifdef DEBUG_PHP
  Serial.printf("url: %s Payload: %s\n", url, response.c_str());
#endif
  return response;
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

