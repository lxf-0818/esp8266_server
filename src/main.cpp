#include <Arduino.h>
#include <ESP8266WiFi.h>

#define PORT 8888
int configSensors(char *sensorName);
void encrypt_stub(char *str, char *str2);
void readSensor(char *Buffer, char *str);
int beginWIFI(String sensorName);

WiFiServer server(PORT);
WiFiClient client;
char Buffer[80], str[80], Buf[80], out_msg[80], encrypt_string[4096];
bool BME_CNFG = false, BMP_CNFG = false, SHT_CNFG=false;
char sensorName[10] = {"no device"};
void setup()
{
 
  Serial.begin(115200);
  configSensors(sensorName);
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
  char tmp[512];
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
    // read data from the connected client
    while (client.available())
      Buffer[j++] = client.read();

    if (!j)
    {
      Serial.println(">>> empty string from client!");
      client.stop();
    }
    readSensor(Buffer, tmp);
    client.print(tmp);
    Serial.println(tmp);

  } // end if client

} // end lo
