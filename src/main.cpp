/**
 * @file main.cpp
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
 * - configSensors(char *sensorName): Configures sensors and returns the count of detected sensors.
 * - printDevice(int deviceNo): Prints I2C address and pin info for a detected sensor to Serial.
 * - encrypt_stub(char *str, char *str2): Wrapper to AES-encrypt a string (defined in login.cpp).
 * - getSensorData(char *cmdFromClient, char *str): Retrieves sensor data based on client commands.
 * - beginWIFI(String sensorName): Initializes WiFi connection with the given sensor name.
 * - performSystemTask(char *cmdFromClient): Executes system tasks based on client commands.
 *
 * @variables
 * - server: WiFiServer instance for handling client connections.
 * - client: WiFiClient instance representing the connected client.
 * - cmdFromClient[80]: Buffer to store commands received from the client.
 * - sensorName[100]: Buffer to store the detected sensor tag list (e.g. BMX_ADC).
 * - results[512]: Response payload buffer sent back to the client.
 * - str[80], Buf[80]: Reserved local buffers (currently unused in loop).
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
#include <ESP8266WiFi.h>
#define DEVICES 5
#include <SimpleFTPServer.h>
char results[512];
int configSensors(char *sensorName);
void encrypt_stub(char *str, char *str2);
void getSensorData(char *cmdFromClient, char *str);
int beginWIFI(String sensorName);
void performSystemTask(char *cmdFromClient);
void printDevice(int deviceNo);
void IRAM_ATTR isr();
void initFz();
FtpServer ftpSrv; // set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial
#define PORT 8888
WiFiServer server(PORT); // port to listen on
WiFiClient client;
char cmdFromClient[80], sensorName[100] = {0}, str[80], Buf[80];
void setup()
{

  int cnt = 0;
  Serial.begin(115200);
  Serial.println("");
  cnt = configSensors(sensorName);
  if (cnt > 0)
  {
    for (int i = 0; i < cnt; i++)
      printDevice(i);

    server.begin();
    beginWIFI(sensorName);
    initFz();
}
else
{

  Serial.println("\n No Valid Sensor Found check wiring");
}

pinMode(LED_BUILTIN, OUTPUT);
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
  ftpSrv.handleFTP(); // make sure in loop you call handleFTP()!!

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
    {
      getSensorData(cmdFromClient, results);
    }
    else
    {
      String IP = WiFi.localIP().toString();
      snprintf(results, sizeof(results), "%s_%s", cmdFromClient, IP.c_str());
      performSystemTask(cmdFromClient);
    }
    client.print(results);
    client.stop();
    // Serial.println(results);

  } // end if client
} //
void ICACHE_RAM_ATTR isr()
{

  for (int i = 0; i < 1; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED on (Note that LOW is the voltage leve
    delay(500);                      // Wait for a second
    digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off by making the voltage HIGH47
    delay(500);
  }
  Serial.println("in isr");
}
