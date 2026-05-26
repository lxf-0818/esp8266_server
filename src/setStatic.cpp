// #include <Arduino.h>
// #include <ESP8266WiFi.h>
// #include <ESP8266HTTPClient.h>
// #include <WiFiClient.h>
// #include <Wire.h>
// // #define DEBUG
// // #define SCREEN_WIDTH 128 // OLED display width, in pixels
// // #define SCREEN_HEIGHT 64 // OLED display height, in pixels
// // #include <Adafruit_SSD1306.h>
// // Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// int tmpConnect(char *ssid, char *psw);
// int setWireBegin(int addr);
// void upDateTableIPstatic(String sensorName);

// #define SSD_ADDR 0x3c

// int setStaticIP(String sensorName, char *ssid, char *psw)

// {
//   int rc = 0, lastOctal;

//   lastOctal = tmpConnect(ssid, psw);
//   if (lastOctal < 180)
//     return lastOctal;
//   String IP = "192.168.1." + lastOctal;
//   IPAddress local_IP(192, 168, 1, lastOctal);
//   IPAddress gateway(192, 168, 1, 1);
//   IPAddress subnet(255, 255, 0, 0);
//   IPAddress primaryDNS(8, 8, 8, 8);   // optional?
//   IPAddress secondaryDNS(8, 8, 4, 4); // optional?

//   // Configures static IP address
//   if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
//   {
//     Serial.println("STA Failed to configure");
//     return 1;
//   }

//   // setWireBegin(SSD_ADDR);
//   // Wire.beginTransmission(SSD_ADDR);
//   // bool SSD_CONFIG = Wire.endTransmission();
//   // if (!SSD_CONFIG)
//   // {
//   //   if (!display.begin(SSD1306_SWITCHCAPVCC, SSD_ADDR)) // Address 0x3D for 128x64
//   //     Serial.println(F("SSD1306 allocation failed"));

//   //   display.clearDisplay();
//   //   display.setTextSize(2);
//   //   display.setTextColor(WHITE);
//   //   display.setCursor(0, 0);
//   //   display.println("server PIO");
//   //   display.println(WiFi.localIP());
//   //   display.println(sensorName);
//   //   display.display();
//   // }
//   return rc;
// }
// int tmpConnect(char *ssid, char *psw)
// {
//   HTTPClient http;
//   WiFiClient client_sql;
//   String apiKeyValue = "tPmAT5Ab3j7F9", sensorLocation = "HOME";
//   char Buf[80];
//   String payload;
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(ssid, psw);
//   Serial.print("Connecting to WiFi ..");
//   while (WiFi.status() != WL_CONNECTED)
//   {
// //    Serial.print('.');
//     delay(1000);
//   }
//   Serial.println(WiFi.localIP());

//   WiFi.macAddress().toCharArray(Buf, sizeof(Buf));
  
//   String serverName = "http://192.168.1.252/isMACinDB.php";
//   String httpRequestData = "api_key=" + apiKeyValue + "&macAddress=" + (String)Buf;
//   http.begin(client_sql, serverName.c_str());
//   http.addHeader("Content-Type", "application/x-www-form-urlencoded");
//   delay(500);
//   int httpResponseCode = http.POST(httpRequestData);
//   payload = http.getString();
//   Serial.printf("http rc %d payload %s mac %s \n", httpResponseCode, payload.c_str(),Buf);
//   payload = http.getString();
//   WiFi.disconnect();
//   http.end();
//   char *token = strtok((char *)payload.c_str(), ",");
//   Serial.printf("payload %s token %s\n",payload.c_str(),token);
//   int pid = atoi(token); //
  
//   Serial.printf("lastOctal %d\n", pid+180);
//   #define DEVICES 6
//   if (pid < 0 || pid > DEVICES)
//     return pid; // ip/mac db empty?
//   return pid + 180; // Set Static IP address
// }
