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
 * - encrypt_stub(char *str, char *str2): Placeholder for encryption functionality.
 * - getSensorData(char *cmdFromClient, char *str): Retrieves sensor data based on client commands.
 * - beginWIFI(String sensorName): Initializes WiFi connection with the given sensor name.
 * - performSystemTask(char *cmdFromClient): Executes system tasks based on client commands.
 * - scanPorts(): Scans available ports (not implemented in the provided code).
 *
 * @variables
 * - server: WiFiServer instance for handling client connections.
 * - client: WiFiClient instance representing the connected client.
 * - cmdFromClient: Buffer to store commands received from the client.
 * - sensorName: Buffer to store the name of the sensor.
 * - str, Buf: General-purpose buffers for string operations.
 *
 * @usage
 * - Upload this code to an ESP8266 microcontroller.
 * - Ensure proper wiring and sensor configuration.
 * - Connect to the ESP8266 server using a client application and send commands.
 *
 * @warning
 * - Ensure the server IP and port are correctly configured.
 * - Handle sensitive data securely if encryption is implemented.
 */
#include <Arduino.h>
#include <ESP8266WiFi.h>

int configSensors(char *sensorName);
void encrypt_stub(char *str, char *str2);
void getSensorData(char *cmdFromClient, char *str);
int beginWIFI(String sensorName);
void performSystemTask(char *cmdFromClient);

void IRAM_ATTR isr();

void IRAM_ATTR isr()
{
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED on (Note that LOW is the voltage leve
    delay(500);                      // Wait for a second
    digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off by making the voltage HIGH47
    delay(500);
  }
}

#define PORT 8888
WiFiServer server(PORT); // port to listen on
WiFiClient client;
char cmdFromClient[80], sensorName[100] = {0}, str[80], Buf[80];
void setup()
{
  int cnt = 0;
  Serial.begin(115200);
  Serial.println("in setup() ");
  cnt = configSensors(sensorName);
  if (cnt > 0)
  {
    server.begin();
    beginWIFI(sensorName);
  }
  else
    Serial.println("\n No Device Found check wiring");
  pinMode(D6, INPUT_PULLUP);
  attachInterrupt(D6, isr, FALLING);
}
void loop()
{
  unsigned j = 0;
  char results[512];
  client = server.accept(); //
  if (client)
  {
    if (client.connected())
    {
      Serial.print(client.remoteIP());
      // Check if the client's IP address is allowed (firewall logic can be added here if needed)
      Serial.println("  Client(esp32) Connected to Server");
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
      {
        cmdFromClient[j++] = toupper(client.read());
      }
      else
      {
        client.read(); // Discard excess data
      }

    if (strnstr(cmdFromClient, "ALL", 3))
    {
      getSensorData(cmdFromClient, results);
    }
    else
    {
      String IP = WiFi.localIP().toString();
      snprintf(results, sizeof(results), "%s_%s", cmdFromClient, IP.c_str());
      client.print(results);
      client.stop();
      performSystemTask(cmdFromClient);
      
    }
    client.print(results);
    client.stop();
    Serial.println(results);

  } // end if client
} //
