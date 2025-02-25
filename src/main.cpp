#include <Arduino.h>
#include <ESP8266WiFi.h>

#define PORT 8888
int configSensors(char *sensorName);
void encrypt_stub(char *str, char *str2);
void readSensor(char *cmdFromClient, char *str);
int beginWIFI(String sensorName);
void performSystemTask(char *cmdFromClient);
void scanPorts();
//String serverName = "http://192.168.1.252/isMACinDB.php";
const char *ipDelete = "http://192.168.1.252/deleteIP.php";

WiFiServer server(PORT); // port to listen on
WiFiClient client;
char cmdFromClient[80], sensorName[100] = {0}, str[80], Buf[80];
void setup()
{
  int cnt = 0;
  Serial.begin(115200);
  while (!Serial);
  //Serial.println("in setup() ");
  cnt = configSensors(sensorName);
  if (cnt > 0)
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
    memset(cmdFromClient, 0, sizeof(cmdFromClient));
    // read data from the connected client
    while (client.available())
      cmdFromClient[j++] = toupper(client.read());

    if (strnstr(cmdFromClient, "ALL", 3)) //
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
