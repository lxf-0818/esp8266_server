/**
 * @file main.cpp
 * Doxygen
 * @brief ESP8266 server implementation for handling client connections and sensor data.
 *
 * This program sets up a WiFi server on the ESP8266 microcontroller to listen for client connections
 * on a specified port. It processes incoming commands from clients, retrieves sensor data, and performs
 * system tasks based on the received commands. The server also handles timeouts and ensures proper
 * communication with connected clients.
 *
 * @details
 * - The server listens on port 8888.
 * - Commands from clients are processed and responded to with appropriate data or actions.
 * - The program includes functionality for configuring sensors, managing WiFi connections, and
 *   performing system tasks.
 * - The server handles client timeouts to ensure stability.
 *
 * @dependencies
 * - Arduino.h: Core Arduino library for ESP8266.
 * - ESP8266WiFi.h: Library for WiFi functionality on ESP8266.
 *
 * @defines
 * - PORT: The port number the server listens on (default: 8888).
 *
 * @functions
 * - setup(): Detects sensors, starts the TCP server, connects to Wi-Fi, starts ElegantOTA.
 * - loop(): Accepts TCP clients, dispatches ALL/non-ALL commands, handles timeout and response.
 * - configSensors(char *sensorName): Probes I2C/1-Wire bus and populates sensorName tag string; returns sensor count.
 * - printDevice(int deviceNo): Prints I2C address and pin info for a detected sensor to Serial.
 * - encrypt_stub(char *str, char *str2): Wrapper to AES-encrypt a string (defined in login.cpp).
 * - getSensorData(char *cmdFromClient, char *str): Builds sensor payload for ALL command into str.
 * - beginWIFI(String sensorName): Connects to Wi-Fi and registers device with the HTTP back-end.
 * - performSystemTask(char *cmdFromClient): Executes BLK/OTA/system tasks based on client command.
 * - initFz(): Initialises the FTP server credentials and begins serving.
 * - isr(): GPIO interrupt handler — blinks LED_BUILTIN once (reserved for future use).
 *
 * @variables
 * - server: WiFiServer instance listening on PORT 8888.
 * - serverAsyn: ESPAsyncWebServer on port 80; hosts ElegantOTA update endpoint.
 * - client: WiFiClient instance representing the currently connected TCP client.
 * - cmdFromClient[80]: Buffer for the command received from the client (force-uppercased).
 * - sensorName[100]: Null-terminated tag string of all detected sensors (e.g. "BMX_ADC").
 * - results[512]: Response payload assembled per request and sent back to the client.
 * - str[80], Buf[80]: Reserved local scratch buffers.
 *
 * @usage
 * - Upload this code to an ESP8266 microcontroller.
 * - Ensure proper wiring and sensor configuration.
 * - Connect to the ESP8266 server using a client application and send commands.
 *
 * @warning
 * - Ensure the server IP and port are correctly configured.
 * - Handle sensitive data securely if encryption is implemented.
 *
 * sample output:
 *  setup()
 *    Connected to NETGEAR37-2
 *    IP address: 192.168.1.3
 *     Port 8888
 *     http rc 200 payload 110|
 *         api_key=xxxxxxx&board=esp8266&location=HOME&IPv4Address=192.168.1.3&macAddress=C8:C9:A3:10:E2:BF&sensor=BMX192.168.1.8  Client(esp32) Connected to Server
 *   loop()
 *     -> address 0x77 sca 5 scl 4
 *     match
 *     clear text      77,73.652734,974.031616
 *     encrypt string: 2dc8Pdrfuqc+eaWr7OdjkXhcX8QVajxeX6plkCf96bY=
 *     b182b3d0:2dc8Pdrfuqc+eaWr7OdjkXhcX8QVajxeX6plkCf96bY=
 */
#include <Arduino.h>
#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "time.h"
#include <Ticker.h>

char results[512], buildTime[20];
int configSensors(char *sensorName);
void encrypt_stub(char *str, char *str2);
void getSensorData(char *cmdFromClient, char *str);
int beginWIFI(String sensorName);
void performSystemTask(char *cmdFromClient, char *results);
void printDevice(int deviceNo);
void IRAM_ATTR isr();
void getBuildTime(char *lastBook);
void upDateTableI2C(String sensorName, int deviceNo);
String performHttpGet(const char *url);
//void lwdtFeed(void);
//#define LWD_TIMEOUT 600 * 1000 // Reboot if loop watchdog timer reaches this time out value
//unsigned long lwdTime = 0;
//unsigned long lwdTimeout = LWD_TIMEOUT;
//void IRAM_ATTR lwdtcb(void);
String phpServerIP;

#define PORT 8888
WiFiServer server(PORT); // port to listen on
WiFiClient client;
AsyncWebServer serverAsyn(80);
char cmdFromClient[80], sensorName[100] = {0}, str[80], Buf[80];
//Ticker lwdTicker;

/**
 * @brief Performs one-time startup for the ESP8266 sensor server.
 *
 * Startup flow:
 * 1. Initializes Serial (115200) and sets `LED_BUILTIN` as output.
 * 2. Detects attached sensors via `configSensors(sensorName)`.
 * 3. If no valid sensor is found, prints a wiring warning and skips server startup.
 * 4. Starts the TCP server on `PORT` (8888) and connects/registers Wi-Fi via `beginWIFI()`.
 * 5. Clears previous I2C rows for this board (`deleteI2C.php?key=<mac>`).
 * 6. Iterates parsed sensor tags, prints each device, and uploads per-device I2C pin mapping using `upDateTableI2C()`.
 * 7. Starts Async HTTP server on port 80, registers `/` route, and enables ElegantOTA.
 *
 * @note Interrupt setup on D6 is intentionally disabled (left commented in source).
 */

void setup()
{

  int cnt = 0;
  Serial.begin(115200);
  Serial.println();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);          // turn off
  //lwdTicker.attach_ms(LWD_TIMEOUT, lwdtcb); // attach lwdt callback routine to Ticker object

  cnt = configSensors(sensorName);
  if (cnt > 0)
  {
    server.begin();
    char macAddr[80];
    WiFi.macAddress().toCharArray(macAddr, sizeof(macAddr));
    Serial.printf("mac @ %s\n", macAddr);
    beginWIFI(sensorName);
    String phpScript = phpServerIP + "deleteI2C.php?key=" + (String)macAddr;
    String payload = performHttpGet(phpScript.c_str());

    // String payload = performHttpGet(phpScript.c_str());
    // Serial.printf("delete mac %s\n", payload.c_str());

    String sensor = sensorName;
    sensor.concat("_");
    int z = 0;
    // Expand the underscore-delimited sensor list into one device per pass.
    while (1)
    {
      int j = sensor.indexOf("_");
      if (j > 0)
      {
        // Current token becomes the sensor family name for this device.
        String name = sensor.substring(0, j);
        printDevice(z);
        upDateTableI2C(name, z);
        // Advance to the remaining tags
        z++;
        sensor = sensor.substring(j + 1);
      }
      else
        break;
    }
    // getBuildTime(buildTime);
    serverAsyn.on("/", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(200, "text/plain", "Hi! I am ESP8266."); });
    serverAsyn.begin();
    ElegantOTA.begin(&serverAsyn);
   // lwdtFeed();
  }
  else
    Serial.println("\n No Valid Sensor Found check wiring");

  // pinMode(D6, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(D6), isr, CHANGE);
}
/**
 * @brief Main loop function to handle client-server communication.
 *
 * This function continuously checks for incoming client connections, processes
 * client requests, and sends appropriate responses. It includes timeout handling
 * for client inactivity and ensures data integrity by discarding excess data.
 *
 * Workflow:
 * 1. Accepts a client connection from the server.
 * 2. Client IP logging is currently disabled (commented out).
 * 3. Waits for data from the client with a timeout of 5 seconds.
 * 4. Reads and processes the client's command, converting it to uppercase.
 * 5. If the command contains "ALL", retrieves sensor data and prepares a response.
 * 6. Otherwise, appends the server's IP address to the command and sends it back.
 * 7. Executes a system task based on the client's command.
 * 8. Sends the response back to the client and closes the connection.
 *
 * @note The function uses a buffer `cmdFromClient` to store the client's command
 *       and ensures it does not overflow by discarding excess data.
 *
 * @warning Ensure that the `getSensorData` and `performSystemTask` functions are
 *          implemented correctly to handle the commands and avoid unexpected behavior.
 *
 * @param None
 * @return None
 */
void loop()
{
  ElegantOTA.loop();
  unsigned j = 0;
  client = server.accept(); //
  if (client)
  {
    if (client.connected())
    {
      // Serial.print(client.remoteIP());
      // Check if the client's IP address is allowed (firewall logic can be added here if needed)
      // Serial.println("  Client(esp32) Connected to Server");
    }
    unsigned long timeout = millis();
    // wait for data to be available
    while (client.available() == 0)
    {
      if (millis() - timeout > 5000)
      {
        Serial.println(">>> server Timeout !");
        client.stop();
        delay(60);
        break;
      }
    }
    memset(cmdFromClient, 0, sizeof(cmdFromClient));
    // read data from the connected client
    while (client.available())
      if (j < sizeof(cmdFromClient) - 1)
        cmdFromClient[j++] = toupper(client.read());
      else
        client.read(); // Discard excess data

    if (strnstr(cmdFromClient, "ALL", 3))
      getSensorData(cmdFromClient, results);
    else
      performSystemTask(cmdFromClient, results);

    client.print(results);
    client.stop();
    Serial.println(results);
    //lwdtFeed();

  } // end if client
} //
// /**
//  * @brief GPIO interrupt service routine (reserved for future use).
//  *
//  * Placed in IRAM via ICACHE_RAM_ATTR. Currently blinks LED_BUILTIN once (500 ms on/off)
//  * a nd prints "in isr" to Serial. The triggering pin (D6, CHANGE) is configured but
//  * the attachInterrupt() call is commented out in setup().
//  */
// void ICACHE_RAM_ATTR isr()
// {

//   for (int i = 0; i < 1; i++)
//   {
//     digitalWrite(LED_BUILTIN, HIGH); // Turn the LED on (Note that LOW is the voltage leve
//     delay(500);                      // Wait for a second
//     digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off by making the voltage HIGH47
//     delay(500);
//   }
//   Serial.println("in isr");
// }

void getBuildTime(char *buildTime)
{
  const char *ntpServer = "pool.ntp.org";
  const long gmtOffset_sec = -18000;
  const int daylightOffset_sec = 3600;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;

  int retries = 3;

  while (retries--)
  {
    if (!getLocalTime(&timeinfo))
    {
      // strncpy(buildTime, FAILED_TO_OBTAIN_TIME, strlen(FAILED_TO_OBTAIN_TIME) + 1);
      Serial.printf("Failed to obtain time retry: %d\n", 3 - retries);
    }
    else
    {
      int hr = timeinfo.tm_hour;

      snprintf(buildTime, 64, "%d/%d/%d %d:%02d ",
               timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_year + 1900,
               hr, timeinfo.tm_min);
      // Serial.printf("string length %d  %d \n",strlen(buildTime),cnt);
      break;
    }
  }
  Serial.printf("Boot time: %s\n", buildTime);
}
// Function accepts a pointer and the size
// void printStrings(std::string arr[], int size) {
//     for (int i = 0; i < size; i++) {
//         //std::cout << arr[i] << " ";
//         Serial.printf("arr %s\n",arr[i]);
//     }
// }
/**
 * @brief Resets the lightweight watchdog timer by updating the current time and timeout.
 *
 * This function sets the `lwdTime` variable to the current time (in milliseconds)
 * and calculates the new timeout value by adding the predefined `LWD_TIMEOUT`
 * to the current time. It ensures that the lightweight watchdog timer does not
 * trigger a timeout as long as this function is called periodically.
 */
// void lwdtFeed(void)
// {
//   lwdTime = millis();
//   lwdTimeout = lwdTime + LWD_TIMEOUT;
// }
/*
This function is a lightweight watchdog callback that checks whether the main loop is still being serviced on time.
The `IRAM_ATTR` attribute places it in instruction RAM, which is typically required for timer/ISR-safe execution
on ESP targets.

The `if` condition has two failure checks joined by `||`.
The first check, `millis() - lwdTime > LWD_TIMEOUT`, detects a missed heartbeat (too much time since the last feed).
The second, `lwdTimeout - lwdTime != LWD_TIMEOUT`, verifies internal timeout bookkeeping consistency. If either condition
is true, the system is considered unhealthy.

On failure, it logs two diagnostic values: actual elapsed time since last feed and the stored timeout delta.
It then immediately calls `ESP.restart()` to recover.
This is a fail-fast recovery path intended to restore operation after stalls or corrupted timing state.
*/
// void IRAM_ATTR lwdtcb(void)
// {
//   if ((millis() - lwdTime > LWD_TIMEOUT) || (lwdTimeout - lwdTime != LWD_TIMEOUT))
//   {
//     // Log elapsed-vs-expected watchdog timing before forcing recovery.
//     Serial.printf("3rd_WDTimer esp.restart %lu %lu\n", (millis() - lwdTime), (lwdTimeout - lwdTime));
//     ESP.restart();
//   }
// }