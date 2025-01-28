#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#define DEBUG
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int tmpConnect(char *ssid, char *psw);
int setWireBegin(int addr);

#define SSD_ADDR 0x3c

const char *getNextIPaddr = "http://192.168.1.252/static_IP.php";

int setStaticIP(String sensorName, char *ssid, char *psw)
{
  Serial.println(sensorName);
  int rc = 0, lastOctal;
#ifndef DEBUG
  Serial.println("in release mode ");
  lastOctal = tmpConnect(ssid, psw);
#else
  Serial.println("in debug mode for sql static ip");
  lastOctal = 181;
#endif
  IPAddress local_IP(192, 168, 1, lastOctal);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 0, 0);
  IPAddress primaryDNS(8, 8, 8, 8);   // optional?
  IPAddress secondaryDNS(8, 8, 4, 4); // optional?

  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    Serial.println("STA Failed to configure");
    return 1;
  }

  setWireBegin(SSD_ADDR);

  Wire.begin(12, 14);
  Wire.beginTransmission(SSD_ADDR);
  bool SSD_CONFIG = Wire.endTransmission();
  if (!SSD_CONFIG)
  {
    if (!display.begin(SSD1306_SWITCHCAPVCC, SSD_ADDR)) // Address 0x3D for 128x64
      Serial.println(F("SSD1306 allocation failed"));

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("server PIO");
    display.println(WiFi.localIP());
    display.println(sensorName);
    display.display();
  }
  Wire.begin(4, 5); // set scl sda to default
  return rc;
}
int tmpConnect(char *ssid, char *psw)
{
  HTTPClient http;
  WiFiClient client;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, psw);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  http.begin(client, getNextIPaddr);
  int httpResponseCode = http.GET();
  Serial.printf("httpResponseCode:%d\n", httpResponseCode);
  if (httpResponseCode != 200)
  {
    //  ESP.restart();
  }
  WiFi.disconnect();
  String payload = http.getString();
  Serial.println(payload);
  http.end();
  return (payload.toInt()) + 179; // Set Static IP address
}