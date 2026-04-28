/**
 **
 * @brief Connects to WiFi, optionally initialises the SSD1306 display, and
 *        registers the device in the remote database.
 *
 * @details
 * 1  Calls `readEncyptWifiCredentials()` to return the ssid:password
 * 2. calls `WiFi.begin()`
 * 3. Waits up to 20 seconds for `WL_CONNECTED`; returns `1` on timeout.
 * 4. Probes I2C address `SSD_ADDR` (0x3C); if an SSD1306 is present, displays
 *    "server PIO", the local IP, and `sensorName`.
 * 5. Issues an HTTP GET to `deleteIP.php` to purge any stale DB entry for
 *    this IP, then calls `upDateDB()` to register the current MAC/IP/sensor.
 *
 * @param sensorName Name of the sensor shown on the display and stored in the DB.
 * @return 0 on success, 1 if the WiFi connection times out.
 *
 * @author Leon Freimour
 */
#include <Arduino.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define SSD_ADDR 0x3c
#define PORT 8888
int setWireBegin(int addr);

int beginWIFI(String sensorName);
int readEncyptWifiCredentials(char *cssid_psw);
void upDateDB(String sensorName);
String performHttpGet(const char *url);
void conver2hexAscii(unsigned char *iv);
int writeLittle(char *fileName, const char *message);


int beginWIFI(String sensorName)
{
  String ssid, pass, temp;
  char cssid_psw[80];
  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 20000; // 20 seconds timeout

  if (readEncyptWifiCredentials(cssid_psw))
    ESP.restart();
  
  temp = cssid_psw;
  int index = temp.indexOf(":"); // get eos token
  ssid = temp.substring(0, index);
  pass = temp.substring(index + 1);

  WiFi.begin(ssid.c_str(), pass.c_str()); // Connect to wifi
  Serial.println("\nConnecting to Wifi");
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
  upDateDB(sensorName); 

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
/**
 * @brief Sends an HTTP GET request and returns the response body.
 *
 * Thin HTTP GET wrapper used by `beginWIFI()` to purge stale IP registrations
 * on boot via `deleteIP.php`.
 *
 * @param url Null-terminated URL string for the GET request.
 * @return String Response body on HTTP 200, or an empty String on any other
 *         status code (the error code is logged to the serial monitor).
 *
 * @note Compile with `-DDEBUG_PHP` to enable verbose URL + payload logging.
 */
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

