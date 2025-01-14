#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define SSD_ADDR 0x3c

int setStaticIP(String sensorName)
{

  int rc = 0;
  // Set your Static IP address
  int lastOctal;
  if (sensorName == "ADS1115")
    lastOctal = 180;
  else if (sensorName == "BME280")
    lastOctal = 181;
  else if (sensorName == "BMP280")
    lastOctal = 191;
  else if (sensorName == "SHT35")
    lastOctal = 182;
  else if (sensorName == "DS18B20")
    lastOctal = 183;
  else
    return 10;

  /* original
    WiFi.mode(WIFI_STA);
 */

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
  Wire.begin(12, 14);
  Wire.beginTransmission(SSD_ADDR);
  bool SSD_CONFIG = Wire.endTransmission();
  if (!SSD_CONFIG)
  {
    if (!display.begin(SSD1306_SWITCHCAPVCC, SSD_ADDR))
    { // Address 0x3D for 128x64
      Serial.println(F("SSD1306 allocation failed"));
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("server PIO");
    display.println(sensorName);
    display.setTextSize(2);
    display.println(WiFi.localIP());
    display.display();
  }
  Wire.begin(4, 5); // set scl sda to default
  return rc;
}
