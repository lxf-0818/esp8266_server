#include <Arduino.h>
#include <ESP8266WiFi.h>

#define PORT 8888
int configSensors(char *sensorName);
void encrypt_stub(char *str, char *str2);
void readSensor(char *cmdFromClient, char *str);
int beginWIFI(String sensorName);
void performSystemTask(char *cmdFromClient);

WiFiServer server(PORT);
WiFiClient client;
char cmdFromClient[80], str[80], Buf[80], out_msg[80], encrypt_string[4096];
char sensorName[10] = {"no device"};

void setup()
{
  Serial.begin(115200);
  int cnt = configSensors(sensorName);
  Serial.printf("# sensor's sensed %d\n", cnt);
  if (strcmp(sensorName, "no device"))
  {
    server.begin();
    beginWIFI(sensorName);
  }
  else
    Serial.println("\n No Device Found check wiring");
}
void loop()
{
  int j = 0;
  char results[512];
  client = server.accept(); //
  if (client)
  {
    if (client.connected())
    {
      Serial.print(client.remoteIP());
      // add firewall
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

    bzero(cmdFromClient, sizeof(cmdFromClient));
    // read data from the connected client
    while (client.available())
      cmdFromClient[j++] = client.read();

    if (strnstr(cmdFromClient, "all", 3))
      readSensor(cmdFromClient, results);
    else 
    {
      performSystemTask(cmdFromClient);
      String IP = WiFi.localIP().toString();
      sprintf(results, "%s_%s", cmdFromClient, IP.c_str());
    }
    client.print(results);
    client.stop();
    Serial.println(results);

  } // end if client

} // end lo
