/**
 * @file oneWire.cpp
 * @brief This file contains functions to interface with OneWire devices, such as DS18B20 temperature sensors, 
 *        using the OneWire library on an ESP8266 microcontroller.
 * 
 * @details
 * - The `scanOneWire` function scans the OneWire bus for connected devices and returns the count of detected devices.
 * - The `readTemp` function reads temperature data from DS18B20 sensors on the OneWire bus and formats the results into a string.
 * - Debugging information is printed to the Serial monitor when the `DEBUG` macro is defined.
 */

 /**
  * @brief Scans the OneWire bus for connected devices.
  * 
  * @return int The number of devices detected on the OneWire bus.
  * 
  * @note If no devices are found, the function will return 0.
  * @note Debugging information is printed to the Serial monitor if the `DEBUG` macro is defined.
  */

 /**
  * @brief Reads temperature data from DS18B20 sensors on the OneWire bus.
  * 
  * @param[out] str A character array to store the formatted temperature data or error messages.
  *                 The format for valid data is "<device_address>,<temperature>|,<device_address>,<temperature>|,...".
  * 
  * @return int Status code:
  *         - 0: Success, temperature data is stored in `str`.
  *         - 1: CRC check for ROM address failed.
  *         - 2: CRC check for scratchpad data failed.
  *         - 3: No devices found or devices dropped, check wiring.
  * 
  * @note The function assumes a maximum of 10 devices on the OneWire bus.
  * @note Debugging information is printed to the Serial monitor if the `DEBUG` macro is defined.
  * @note Temperatures are converted to Fahrenheit.
  */
#include <Arduino.h>
#include <OneWire.h>

#define DEBUG
int readTemp(char *str);
int scanOneWire();

const int oneWireBus = 4; // d2 on esp8266
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

int scanOneWire()
{
  int deviceCount = 0;
  byte addr[8];
 // Serial.printf("start DS18B20 \n");

  while (1)
  {
    if (!oneWire.search(addr))
    {
      if (deviceCount)
      {
#ifdef DEBUG
        Serial.printf("DS18B20 detected cnt:%d\n", deviceCount);
#endif
        oneWire.reset_search();
      }

      break;
    }
    deviceCount++;
  }
  return deviceCount;
}

int readTemp(char *str)
{
  byte data[12], addr[8], present = 0;
  float temp[10];
  int deviceCount = 0;

  while (1)
  {
    if (!oneWire.search(addr))
    {
      oneWire.reset_search();
      if (!deviceCount)
        strcpy(str, "devices dropped check wiring");
      break;
    }

    if (OneWire::crc8(addr, 7) != addr[7])
    {
      strcpy(str, "CRC ROM is not valid!");
#ifdef DEBUG
      Serial.print("ROM =");
      for (int i = 0; i < 8; i++)
      {
        Serial.write(' ');
        Serial.print(addr[i], HEX);
      }
#endif
      return 1;
    }

    oneWire.reset();
    oneWire.select(addr);
    oneWire.write(0x44, 1); // start conversion, with parasite power on at the end
    delay(1000);            // maybe 750ms is enough?
    present = oneWire.reset();
    oneWire.select(addr);
    oneWire.write(0xBE);        // Read Scratchpad
    for (int i = 0; i < 9; i++) // we need 9 bytes
      data[i] = oneWire.read();

    if (OneWire::crc8(data, 8) == data[8])
    {
      int16_t raw = (data[1] << 8) | data[0]; // swap bytes
      temp[deviceCount++] = ((float)raw / 16.0) * 1.8 + 32;
    }
    else
    {
      strcpy(str, "CRC RAW is not valid!");
#ifdef DEBUG
      Serial.print("  Data = ");
      Serial.print(present, HEX);
      Serial.print(" ");
      for (int i = 0; i < 9; i++)
      { // we need 9 bytes
        data[i] = oneWire.read();
        Serial.print(data[i], HEX);
        Serial.print(" ");
      }
#endif
      return 2;
    }
  }

  int offset = 0;
  for (int i = 0; i < deviceCount; i++)
  {
    sprintf(str + offset, "%x,%f,|,", addr[0], temp[i]);
    offset += strlen(str);
  }
  if (deviceCount)
    str[(strlen(str) - 1)] = '0'; // remove last ','
  else
  {
    strcpy(str, "device(s) were dropped check wiring");
    return 3;
  }

  return 0;
}
