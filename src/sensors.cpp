/**
 * @file sensors.cpp
 * @brief This file contains the implementation of sensor configuration, initialization,
 *        and data retrieval for various I2C and OneWire sensors connected to an ESP8266.
 *
 * The code supports multiple sensors, including BMP3XX, BME280, BMP280, SHT35, ADS1115,
 * and DS18B20. It provides functionality to scan I2C ports, initialize sensors, and
 * retrieve sensor data in a formatted string.
 *
 * @details
 * - The `configSensors` function initializes the sensors and builds a list of detected
 *   sensors.
 * - The `getSensorData` function retrieves data from configured sensors, optionally AES-encrypts
 *   the payload, calculates CRC32 over the transmitted payload, and appends the IV.
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
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include "Adafruit_BMP3XX.h"
#include "Adafruit_BME680.h"
#include <Adafruit_ADS1X15.h>
#include "Adafruit_INA219.h"
#include <SHT85.h>
#include <Adafruit_Sensor.h>
#include <AESLib.h>
#include <CRC.h>
#include <ArduinoJson.h>
#include <map>

// #define DEBUG_SCAN
#define SOCKET_AES

#define SHT_ADDRESS 0x44
#define BMx_ADDRESS 0x76
#define BMPX_ADDRESS 0x77
#define BMP_ADDRESS 0x58
#define ADC_ADDRESS 0x48
#define MCP_ADDRESS 0x18
#define INA_ADDRESS 0x40
#define DEVICES 8
#define SEALEVELPRESSURE_HPA (1012.8)

int configSensors(char *sensorName);
void getSensorData(char *cmd, char *str);
int setWireBegin(int addr);
int scanOneWire();
int check_if_exist_I2C(uint8_t i, uint8_t j);
void scanI2Cports();
int readTemp(char *str);
void encrypt_stub(char *str, char *str2);
String convert2hexAscii(unsigned char *iv);
int readAES(char *fileName, byte data[]);
extern byte aes_iv[];
void dumpI2C(char *result);

// Valid pins for I2C
String portMap[] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7"};
uint8_t portArray[] = {16, 5, 4, 0, 2, 14, 12, 13};
int myCnt = 0;

// I2C
Adafruit_BME280 bme;
Adafruit_BMP280 bmp;
Adafruit_BMP3XX bmp3xx;
Adafruit_BME680 bme680; // I2C
Adafruit_ADS1115 adc;
Adafruit_INA219 ina219;

SHT35 sht;

bool BME_CNFG = false, BMP_CNFG = false, BMX_CNFG = false,
     ADC_CNFG = false, DS1_CNFG = false, SHT_CNFG = false,
     INA_CNFG = false, BM6_CNFG = false;
struct I2C
{
    int I2Caddr;
    int scl;
    char sclGPIO[3];
    int sca;
    char scaGPIO[3];
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
    setWireBegin(BMPX_ADDRESS);
    if (bme680.begin())
    {
        sensorArray[sensorsInstalled] = "BM6";
        BM6_CNFG = true;
        sensorsInstalled++;
    }
    setWireBegin(BMPX_ADDRESS);
    if (bmp3xx.begin_I2C(BMPX_ADDRESS))
    {
        sensorArray[sensorsInstalled] = "BMX";
        BMX_CNFG = true;
        sensorsInstalled++;
    }
    setWireBegin(INA_ADDRESS);
    if (ina219.begin())
    {
        sensorArray[sensorsInstalled] = "INA";
        INA_CNFG = true;
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
 * @brief Collects sensor data from configured sensors, optionally encrypts it,
 *        computes CRC32, and emits payload plus IV.
 *
 * @param cmd Unused parameter, reserved for future use.
 * @param str Pointer to a character buffer where the resulting data or error message will be stored.
 *
 * The function performs the following steps:
 * 1. Checks the configuration flags for each sensor type (e.g., BMX, BME, BMP, SHT, ADC, DS1).
 * 2. Reads data from the corresponding sensors if they are configured and available.
 * 3. Formats the sensor data into a specific string format and concatenates it.
 * 4. Optionally encrypts the concatenated sensor string when SOCKET_AES is enabled.
 * 5. Calculates a CRC32 checksum of the outgoing payload (`tmp`), encrypted or plain.
 * 6. Appends the active AES IV as a hex-comma string and stores final output in `str`.
 *
 * Notes:
 * - If no sensors are configured or available, the function sets `str` to "no sensors found".
 * - The function removes the trailing ",|," from the concatenated sensor data before processing.
 * - The encryption step is controlled by the `SOCKET_AES` macro.
 * - The returned frame format is always `<CRC32_HEX>:<ciphertext>:<iv_hex_csv>`.
 *
 * Example output format (unencrypted): `<CRC32>:<sensor_data>:<iv_hex_csv>`
 * Example output format (encrypted): `<CRC32>:<base64_aes_payload>:<iv_hex_csv>`
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
    String sensorsData;
    int deviceCnt = 0;

    if (BM6_CNFG && setWireBegin(BMPX_ADDRESS))
    {
          sprintf(str, "%x,%f,%u,%f,|,", BMPX_ADDRESS+1,
                bme680.readTemperature() * 1.8 + 32,
                bme680.gas_resistance / 1000,
                bme680.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
        deviceCnt++;
      //  Serial.printf("bme680 %s\n", str);
    }
    if (BMX_CNFG && setWireBegin(BMPX_ADDRESS))
    {
        sprintf(str, "%x,%f,%u,%f,|,", BMPX_ADDRESS,
                bmp3xx.readTemperature() * 1.8 + 32,
               (uint32_t) bmp3xx.readPressure() / 100,
                bmp3xx.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
        deviceCnt++;
    }
    if (BME_CNFG && setWireBegin(BMx_ADDRESS))
    {
        sprintf(str, "%x,%f,%f,%f,|,", BMx_ADDRESS, (bme.readTemperature()) * 1.8 + 32,
                bme.readHumidity(), bme.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
        deviceCnt++;
    }

    if (BMP_CNFG && setWireBegin(BMx_ADDRESS))
    {

        sprintf(str, "%x,%f,%f,|,", BMP280_CHIPID, (bmp.readTemperature()) * 1.8 + 32,
                bmp.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
        deviceCnt++;
    }
    if (INA_CNFG && setWireBegin(INA_ADDRESS))
    {

        sprintf(str, "%x,%f,%f,|,", INA219_ADDRESS, ina219.getBusVoltage_V(), ina219.getCurrent_mA());
        sensorsData.concat(str);
        Serial.printf("busV %f I %f\n", ina219.getBusVoltage_V(), ina219.getCurrent_mA());
        deviceCnt++;
    }
    if (SHT_CNFG && setWireBegin(SHT_ADDRESS))
    {
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
    if (ADC_CNFG & setWireBegin(ADC_ADDRESS))
    {
        float rRatio = 5.63;                                         // rRatio = (r1+r2)/r2  where r1 = 220k r2 =47k
        float volts0 = adc.computeVolts(adc.readADC_SingleEnded(0)); // jackery is connected to A0
        float volts1 = adc.computeVolts(adc.readADC_SingleEnded(1)); // A1 is connected to 3V on ESP
        sprintf(str, "%x,%f,%f,%f|,", ADC_ADDRESS, volts0 * rRatio, volts1, rRatio);
        // Serial.printf("A1 is connected to 3V on ESP\n");
        sensorsData.concat(str);
        deviceCnt++;
    }
    if (DS1_CNFG) // 1-wire device
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
    // Serial.printf("sensor data %s\n", sensorsData.c_str());
#ifdef SOCKET_AES
    // only the payload gets encrypted
    encrypt_stub((char *)sensorsData.c_str(), tmp);
#else
    strcpy(tmp, sensorsData.c_str());
#endif
    uint32_t calcCRC = calcCRC32((uint8_t *)&tmp, strlen(tmp));
#ifdef SOCKET_AES
    String iv = convert2hexAscii(aes_iv);
    bzero(str, 512);
    sprintf(str, "%x:%s:%s", calcCRC, tmp, iv.c_str());
#else
    sprintf(str, "%x:%s", calcCRC, tmp);
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
 * @warning This implementation assumes `portArray` remains a fixed local array.
 *          If converted to a pointer, the `sizeof(portArray)` loops must be rewritten.
 */
void scanI2Cports()
{
    for (uint8_t i = 0; i < sizeof(portArray); i++)
    {
        for (uint8_t j = 0; j < sizeof(portArray); j++)
        {
            if (i != j)
            {
                Wire.begin(portArray[i], portArray[j]);
                if (check_if_exist_I2C(i, j))
                {

// #define DEBUG_SCAN
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
 * @brief Probes the currently selected I2C bus for supported sensor addresses.
 *
 * The function checks only the fixed list of supported device addresses used by
 * this project (`0x44`, `0x76`, `0x18`, `0x58`, `0x48`, `0x77`). For each address
 * that acknowledges on the bus, it records the address together with the active
 * SDA/SCL pin pair into the global `devices` table and increments the device count.
 * The table is appended to using the global `myCnt` index, so the caller should
 * clear or reuse that state only in the surrounding scan workflow.
 *
 * If the I2C transaction reports error code `4`, the ESP8266 is reset immediately.
 *
 * @note This routine depends on `Wire.begin(...)` having already selected the
 *       candidate SDA/SCL pair before probing is called.
 * @note The stored SDA/SCL pins come from the global `i` and `j` indices, so the
 *       caller should only use this helper inside the scan loops that set them.
 *
 * @return int Number of supported I2C devices detected on the active bus.
 *
 * @warning The scan table is appended to `devices[myCnt]`; ensure `myCnt` stays
 *          within bounds for the number of discovered devices.
 */
// #define DEBUG_SCAN
int check_if_exist_I2C(uint8_t i, uint8_t j)
{
    byte error;
    int nDevices = 0;
    uint8_t supportedSensors[] = {0x44, 0x76, 0x18, 0x58, 0x48, 0x77, 0x40};
    for (uint8_t index = 0; index < sizeof(supportedSensors); index++)
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge that address.
        Wire.beginTransmission(supportedSensors[index]);
        error = Wire.endTransmission();
        if (!error)
        {
// #define DEBUG_SCAN
#ifdef DEBUG_SCAN
            Serial.printf("I2C device addr ");
            if (supportedSensors[index] < 16)
                Serial.print("0");

            Serial.print("  0x");
            Serial.println(supportedSensors[index], HEX);
#endif

            //    Serial.printf("valid sensor %x\n", supportedSensors[index]);
            devices[myCnt].I2Caddr = supportedSensors[index];
            devices[myCnt].sca = portArray[i];
            strcpy(devices[myCnt].scaGPIO, portMap[i].c_str());
            strcpy(devices[myCnt].sclGPIO, portMap[j].c_str());
            devices[myCnt++].scl = portArray[j];

            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknow error at address 0x");
            ESP.reset();
        }
    }
    if (nDevices)
        Serial.printf("# of sensors found %d\n", nDevices);

    return nDevices;
}
// Looks up `addr` in the populated `devices` table and calls Wire.begin() with the
// matching SDA/SCL pins for that device. Returns 1 if the address was found, 0 otherwise.
int setWireBegin(int addr)
{
    for (int j = 0; j < DEVICES; j++)
    {
        if (addr == devices[j].I2Caddr)
        {
// #define DEBUG_SCAN
#ifdef DEBUG_SCAN
            Serial.printf("-> address 0x%x sca %d scl %d \n", addr, devices[j].sca, devices[j].scl);
#endif
            Wire.begin(devices[j].sca, devices[j].scl);
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Converts a 16-byte IV buffer into comma-separated lowercase hex ASCII.
 *
 * Each byte is formatted as `xx,` and written into a fixed local buffer. The
 * trailing comma is replaced with a null terminator before the result is
 * returned as an Arduino `String`.
 *
 * Example output:
 * `01,af,3c,00,...,7e`
 *
 * @param iv Pointer to the IV byte array (expected length: 16 bytes).
 * @return String Comma-separated hex representation of the IV.
 */
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

// Prints one populated `devices[]` entry to Serial: I2C address plus mapped SDA/SCL pins.
void printDevice(int deviceNo)
{
    Serial.printf("I2c addr 0x%x sca %s(%d) scl %s(%d) \n",
                  devices[deviceNo].I2Caddr,
                  devices[deviceNo].scaGPIO, devices[deviceNo].sca,
                  devices[deviceNo].sclGPIO, devices[deviceNo].scl);
}
void dumpI2C(char *results)
{

    int deviceNo = 0;
    String tmp;
    tmp.clear();
    // 76:D2(4),D1(5)|77:D2(3),D5(14)|

    while (devices[deviceNo].I2Caddr)
    {
        sprintf(results, "%x:%s(%d),%s(%d)|",
                devices[deviceNo].I2Caddr,
                devices[deviceNo].scaGPIO, devices[deviceNo].sca,
                devices[deviceNo].sclGPIO, devices[deviceNo].scl);
        tmp.concat(results);

        deviceNo++;
    }
    strcpy(results, tmp.c_str());
}
