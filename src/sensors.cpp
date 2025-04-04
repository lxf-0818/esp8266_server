/**
 * @file sensors.cpp
 * @brief This file contains the implementation of sensor configuration, initialization,
 *        and data retrieval for various I2C and OneWire sensors connected to an ESP8266.
 *
 * The code supports multiple sensors, including BMP3XX, BME280, BMP280, SHT85, ADS1115,
 * and DS18B20. It provides functionality to scan I2C ports, initialize sensors, and
 * retrieve sensor data in a formatted string.
 *
 * @details
 * - The `configSensors` function initializes the sensors and builds a list of detected
 *   sensors.
 * - The `getSensorData` function retrieves data from the configured sensors and formats
 *   it into a string with CRC32 checksum.
 * - The `scanPorts` function scans all possible I2C port combinations to detect connected
 *   devices.
 * - The `check_if_exist_I2C` function checks for the presence of I2C devices on the bus.
 * - The `setWireBegin` function sets the I2C pins for a specific device address.
 *
 * @note The code uses global flags to track the configuration status of each sensor.
 *       It also supports optional AES encryption for the sensor data.
 *
 * @dependencies
 * - Arduino core for ESP8266
 * - Adafruit Sensor libraries (Adafruit_BMP3XX, Adafruit_BME280, Adafruit_BMP280, Adafruit_ADS1X15)
 * - SHT85 library
 * - AESLib for encryption
 * - CRC library for checksum calculation
 *
 * @author Leon Freimour
 * @date 2025-3-28
 */
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include "Adafruit_BMP3XX.h"
#include <Adafruit_ADS1X15.h>
#include <SHT85.h>
#include <Adafruit_Sensor.h>
#include <AESLib.h>
#include <CRC.h>
#define SHT_ADDRESS 0x44
#define BMx_ADDRESS 0x76
#define BMPX_ADDRESS 0x77
#define ADC_ADDRESS 0x48
#define MCP_ADDRESS 0x18
#define DEVICES 5
#define NO_SOCKET_AES
#define SEALEVELPRESSURE_HPA (1012.8)

// #define DEBUG_SCAN

int configSensors(char *sensorName);
void getSensorData(char *cmd, char *str);
int setWireBegin(int addr);
int scanOneWire();
int check_if_exist_I2C();
void scanPorts();
int readTemp(char *str);

uint8_t i, j;
uint8_t portArray[] = {16, 5, 4, 0, 2, 14, 12, 13};
String portMap[] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7"};
int myCnt = 0;

// String portMap[] = {"GPIO16", "GPIO5", "GPIO4", "GPIO0", "GPIO2", "GPIO14", "GPIO12", "GPIO13"};

// I2C
Adafruit_BME280 bme;
Adafruit_BMP280 bmp;
Adafruit_BMP3XX bmp3xx;
Adafruit_ADS1115 adc;
SHT35 sht;

bool BME_CNFG = false, BMP_CNFG = false, BMX_CNFG = false,
     ADC_CNFG = false, DS1_CNFG = false, SHT_CNFG = false;
struct I2C
{
    int I2Caddr;
    int scl;
    int sca;
};
struct I2C devices[DEVICES];

/**
 * @brief Configures and initializes various sensors connected to the system.
 *
 * This function scans for connected sensors, initializes them, and updates their
 * configuration flags. It also builds a concatenated string of sensor names
 * representing the installed sensors.
 *
 * @param sensorName A character array to store the concatenated names of the
 *                   detected and configured sensors. The names are separated by underscores.
 *
 * @return The number of sensors successfully detected and configured.
 *
 * @note The function relies on specific sensor addresses and chip IDs to identify
 *       and initialize the sensors. It also assumes the presence of global flags
 *       (e.g., BMX_CNFG, BME_CNFG) to track the configuration status of each sensor.
 *
 * @details The function performs the following steps:
 *          1. Scans for available I2C ports.
 *          2. Attempts to initialize each sensor (BMP3XX, BME, BMP280, SHT, ADC, DS18B20).
 *          3. Updates the sensor configuration flags and stores the sensor name in an array.
 *          4. Concatenates the names of all detected sensors into the provided `sensorName` parameter.
 *          5. Returns the total count of successfully configured sensors.
 */
int configSensors(char *sensorName)
{
    int sensorsInstalled = 0;
    String s, sensorArray[DEVICES];

    scanPorts();
    setWireBegin(BMPX_ADDRESS);
    if (bmp3xx.begin_I2C(BMPX_ADDRESS))
    {
        sensorArray[sensorsInstalled] = "BMX";
        BMX_CNFG = true;
        sensorsInstalled++;
    }

    setWireBegin(BMx_ADDRESS);
    if (bme.begin(BMx_ADDRESS))
    {
        sensorArray[sensorsInstalled] = "BME";
        BME_CNFG = true;
        sensorsInstalled++;
    }
    if (bmp.begin(BMx_ADDRESS, BMP280_CHIPID))
    {
        sensorArray[sensorsInstalled] = "BMP";
        BMP_CNFG = true;
        sensorsInstalled++;
    }
    setWireBegin(SHT_ADDRESS);
    if (sht.begin(SHT_ADDRESS))
    {
        sensorArray[sensorsInstalled] = "SHT";
        SHT_CNFG = true;
        sensorsInstalled++;
    }
    setWireBegin(ADC_ADDRESS);
    if (adc.begin(ADC_ADDRESS))
    {
        adc.setGain(GAIN_ONE); // 1x gain: ADC input range is +/- 4.096V, resolution is 1 bit = 0.125mV
        sensorArray[sensorsInstalled] = "ADC";
        ADC_CNFG = true;
        sensorsInstalled++;
    }
    int ds1Cnt = scanOneWire();
    for (int i = 0; i < ds1Cnt; i++)
    {
        sensorArray[sensorsInstalled++] = "DS1";
        DS1_CNFG = true;
    }

    for (int j = 0; j < sensorsInstalled; j++)
    {
        if (j > 0)
            strcat(sensorName, "_");
        strcat(sensorName, sensorArray[j].c_str());
    }

    return sensorsInstalled;
}

void getSensorData(char *cmd, char *str)
{
    char tmp[512];
    String sensorsData;
    int deviceCnt = 0;

    (void)cmd;

    if (BMX_CNFG)
    {
        setWireBegin(BMPX_ADDRESS);
        sprintf(str, "%x,%f,%f,|,", BMPX_ADDRESS,
                bmp3xx.readTemperature() * 1.8 + 32,
                bmp3xx.readPressure() / 100);
        sensorsData.concat(str);
        deviceCnt++;
    }
    if (BME_CNFG)
    {
        setWireBegin(BMx_ADDRESS);
        sprintf(str, "%x,%f,%f,%f,|,", BMx_ADDRESS, (bme.readTemperature()) * 1.8 + 32,
                bme.readHumidity(), bme.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
        deviceCnt++;
    }
    if (BMP_CNFG)
    {
        setWireBegin(BMx_ADDRESS);
        sprintf(str, "%x,%f,%f,|,", BMP280_CHIPID, (bmp.readTemperature()) * 1.8 + 32,
                bmp.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
        deviceCnt++;
    }
    if (SHT_CNFG)
    {
        setWireBegin(SHT_ADDRESS);
        if (sht.dataReady())
        {
            sht.read(); // default = true/fast       slow = false
            sprintf(str, "%x,%f,%f,|,", SHT_ADDRESS, sht.getFahrenheit(), sht.getHumidity());
            sensorsData.concat(str);
            deviceCnt++;
        }
        else
            strcpy(str, "SHT data not ready");
    }
    if (ADC_CNFG)
    {
        setWireBegin(0x48);
        float volts0 = adc.computeVolts(adc.readADC_SingleEnded(0));
        float volts1 = adc.computeVolts(adc.readADC_SingleEnded(1));
        sprintf(str, "%x,%f,%f,|,", ADC_ADDRESS, volts0, volts1);
        Serial.printf("A0/A1 is connected to 3V on ESP\n");
        sensorsData.concat(str);
        deviceCnt++;
    }
    if (DS1_CNFG)
    {
        int rc = readTemp(str);
        Serial.printf("rc %d\n", rc);
        sensorsData.concat(str);
        deviceCnt++;
    }

    if (!deviceCnt)
        strcpy(str, "0");

    sensorsData = sensorsData.substring(0, sensorsData.length() - 3); // remove the last ",|,"
    strcpy(tmp, sensorsData.c_str());
    uint8_t *data = (uint8_t *)&tmp[0]; // ptr to 1st char in str
    uint32_t t = calcCRC32(data, strlen(tmp));
    bzero(tmp, 512);
    sprintf(tmp, "%x:%s", t, sensorsData.c_str());
#ifndef NO_SOCKET_AES
    encrypt_stub(str, encrypt_string);
    Serial.printf("in server aes %s", encrypt_string);
#else
    strcpy(str, tmp);
#endif
}

void scanPorts()
{
    for (i = 0; i < sizeof(portArray); i++)
    {
        for (j = 0; j < sizeof(portArray); j++)
        {
            if (i != j)
            {
                Wire.begin(portArray[i], portArray[j]);
                if (check_if_exist_I2C())
                {
#ifdef DEBUG_SCAN
                    Serial.printf(" SDA %s(%d) SCL %s(%d) \n",
                                  portMap[i].c_str(), portArray[i], portMap[j].c_str(), portArray[j]);
#endif
                }
            }
        }
    }
}

int check_if_exist_I2C()
{
    byte error, address;
    int nDevices = 0;
    for (address = 1; address < 127; address++)
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (!error)
        {
#ifdef DEBUG_SCAN
            Serial.printf("I2C device addr ");
            if (address < 16)
                Serial.print("0");

            Serial.print("  0x");
            Serial.println(address, HEX);
#endif
            devices[myCnt].I2Caddr = address;
            devices[myCnt].sca = portArray[i];
            devices[myCnt++].scl = portArray[j];

            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknow error at address 0x");
            ESP.reset();
        }
    }
    return nDevices;
}
int setWireBegin(int addr)
{
    for (int j = 0; j < DEVICES; j++)
    {
        if (addr == devices[j].I2Caddr)
        {
#ifdef DEBUG_SCAN
            Serial.printf("-> address 0x%x sca %d scl %d \n", addr, devices[j].sca, devices[j].scl);
#endif
            Wire.begin(devices[j].sca, devices[j].scl);
            return 1;
        }
    }
    return 0;
}
