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
 * - The `scanI2Cports` function scans all possible I2C port combinations to detect connected
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
#include <ArduinoJson.h>

#define DEBUG_SCAN
// #define NO_SOCKET_AES

#define SHT_ADDRESS 0x44
#define BMx_ADDRESS 0x76
#define BMPX_ADDRESS 0x77
#define BMP_ADDRESS 0x58
#define ADC_ADDRESS 0x48
#define MCP_ADDRESS 0x18
#define DEVICES 5
#define SEALEVELPRESSURE_HPA (1012.8)

int configSensors(char *sensorName);
void getSensorData(char *cmd, char *str);
int setWireBegin(int addr);
int scanOneWire();
int check_if_exist_I2C();
void scanI2Cports();
int readTemp(char *str);
void encrypt_stub(char *str, char *str2);
String convert2hexAscii(unsigned char *iv);

// tmp iv before gen iv
int readAES(char *fileName, byte data[]);

// Valid pins for I2C
String portMap[] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7"};
uint8_t portArray[] = {16, 5, 4, 0, 2, 14, 12, 13};
int myCnt = 0;
uint8_t i, j;

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

    int ds1Cnt = scanOneWire();
    for (int z = 0; z < ds1Cnt; z++)
    {
        sensorArray[sensorsInstalled++] = "DS1";
        DS1_CNFG = true;
    }

    scanI2Cports();
    // Wire.begin(5, 4);
    // setWireBegin(0x57);
    //   if (!pox.begin(PULSEOXIMETER_DEBUGGINGMODE_PULSEDETECT)) {
    //     Serial.println("ERROR: Failed to initialize pulse oximeter");
    //     for(;;);
    // }

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
    if (bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID))
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

    for (int j = 0; j < sensorsInstalled; j++)
    {
        if (j > 0)
            strcat(sensorName, "_");
        strcat(sensorName, sensorArray[j].c_str());
    }

    return sensorsInstalled;
}

/**
 * @brief Collects sensor data from various configured sensors, formats the data,
 *        calculates a CRC32 checksum, and optionally encrypts the result.
 *
 * @param cmd Unused parameter, reserved for future use.
 * @param str Pointer to a character buffer where the resulting data or error message will be stored.
 *
 * The function performs the following steps:
 * 1. Checks the configuration flags for each sensor type (e.g., BMX, BME, BMP, SHT, ADC, DS1).
 * 2. Reads data from the corresponding sensors if they are configured and available.
 * 3. Formats the sensor data into a specific string format and concatenates it.
 * 4. Calculates a CRC32 checksum for the concatenated sensor data.
 * 5. Optionally encrypts the resulting string if encryption is enabled.
 * 6. Stores the final result in the provided `str` buffer.
 *
 * Notes:
 * - If no sensors are configured or available, the function sets `str` to "0".
 * - The function removes the trailing ",|," from the concatenated sensor data before processing.
 * - The encryption step is controlled by the `NO_SOCKET_AES` macro.
 *
 * Example output format (unencrypted): `<CRC32>:<sensor_data>`
 * Example sensor data format: `<address>,<value1>,<value2>,|,`
 *
 * Dependencies:
 * - The function relies on external sensor libraries and configuration macros.
 * - The `calcCRC32` function is used to compute the checksum.
 * - The `encrypt_stub` function is used for encryption if enabled.
 */
void getSensorData(char *cmd, char *str)
{
    (void)cmd; // stop warning error
    char tmp[512];
    //  char encrypt_string[512] = {'\0'}; // Declare encrypt_string
    String sensorsData;
    int deviceCnt = 0;

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
        //  Serial.printf("bme config %s\n",str);
        deviceCnt++;
    }
    if (BMP_CNFG)
    {
        setWireBegin(BMP280_CHIPID);
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
        float rRatio = 5.63; // rRatio = (r1+r2)/r2  where r1 = 220k r2 =47k
        setWireBegin(ADC_ADDRESS);
        float volts0 = adc.computeVolts(adc.readADC_SingleEnded(0)); // jackery is connected to A0
        float volts1 = adc.computeVolts(adc.readADC_SingleEnded(1)); // A1 is connected to 3V on ESP
        sprintf(str, "%x,%f,%f,%f|,", ADC_ADDRESS, volts0, volts1, rRatio);
        // Serial.printf("A1 is connected to 3V on ESP\n");
        sensorsData.concat(str);
        deviceCnt++;
    }
    if (DS1_CNFG)
    {
        readTemp(str);
        sensorsData.concat(str);
        deviceCnt++;
    }

    if (!deviceCnt)
    {
        strcpy(str, "no sensors found");
        return;
    }

    sensorsData = sensorsData.substring(0, sensorsData.length() - 3); // remove the last ",|,"
#ifndef NO_SOCKET_AES
    encrypt_stub((char *)sensorsData.c_str(), tmp);
#else
    strcpy(tmp, sensorsData.c_str());
#endif
    uint8_t *data = (uint8_t *)&tmp[0]; // ptr to 1st char in str
    uint32_t calcCRC = calcCRC32(data, strlen(tmp));
#ifndef NO_SOCKET_AES
  //  AESLib aesLib;
    byte aes_iv[N_BLOCK];
    String iv;
  // aesLib.gen_iv(aes_iv);
    // testing only use above in 0roduction
      readAES((char *)"/iv.txt", aes_iv);
    iv = convert2hexAscii(aes_iv);
    sprintf(str, "%x:%s:%s", calcCRC, tmp, iv.c_str());
#else
    bzero(str, 512);
    sprintf(str, "%x:%s", calcCRC, sensorsData.c_str());
#endif
}

/**
 * @brief Scans all possible combinations of SDA and SCL pins from the portArray
 *        to detect connected I2C devices.
 *
 * This function iterates through all combinations of pins in the portArray,
 * excluding cases where the same pin is used for both SDA and SCL. For each
 * combination, it initializes the I2C bus using Wire.begin() and checks if
 * an I2C device exists on the bus using the check_if_exist_I2C() function.
 *
 * If DEBUG_SCAN is defined, the function logs the detected SDA and SCL pin
 * combinations along with their corresponding names from the portMap array
 * to the Serial output.
 *
 * @note The portArray and portMap arrays, as well as the check_if_exist_I2C()
 *       function, must be defined elsewhere in the program.
 *
 * @warning Ensure that the size of portArray is correctly calculated, as
 *          sizeof(portArray) may not work as expected if portArray is a pointer.
 */
void scanI2Cports()
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

/**
 * @brief Scans the I2C bus for connected devices and returns the count of detected devices.
 *
 * This function iterates through all possible I2C addresses (1 to 126) and checks if a device
 * acknowledges communication at each address. If a device is found, its address and associated
 * port information are stored in the `devices` array. The function also handles errors during
 * communication and resets the ESP device if an unknown error occurs.
 *
 * @note The function uses the Wire library for I2C communication. Ensure that the Wire library
 *       is properly initialized before calling this function.
 *
 * @return int The number of I2C devices detected on the bus.
 *
 * @warning If an unknown error occurs during communication, the ESP device will reset.
 *
 * @details
 * - If the `DEBUG_SCAN` macro is defined, the function will print the addresses of detected
 *   I2C devices to the Serial monitor.
 * - The `devices` array and `myCnt` variable are used to store information about detected
 *   devices. Ensure these are properly defined and initialized in the global scope.
 * - The `portArray` array and `i`, `j` indices are used to associate I2C addresses with
 *   specific ports. Ensure these are defined and initialized appropriately.
 */
int check_if_exist_I2C()
{
    byte error, address;
    int nDevices = 0;
    for (address = 1; address < 127; address++)
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge the address.
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
String convert2hexAscii(unsigned char *iv)
{
    char hexAscii[49]; // 16*3 +1
    for (int i = 0; i < 16; i++)
    {
        sprintf(hexAscii + (i * 3), "%02x,", iv[i]);
    }
    hexAscii[47] = '\0'; // remove last coma "," replace with null

    String returnString = hexAscii;
    return returnString;
}