#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_ADS1X15.h>
#include <SHT85.h>
#define DEVICE 5

#include <AESLib.h>
#include <CRC.h>
#define SHT_ADDRESS 0x44
#define BMx_ADDRESS 0x76
#define ADC_ADDRESS 0x48
#define MCP_ADDRESS 0x18

#define NO_SOCKET_AES
#define SEALEVELPRESSURE_HPA (1013.25)
#define DEBUG_SCAN
int myCnt = 0;
int configSensors(char *sensorName);
void readSensor(char *cmd, char *str);
int setWireBegin(int addr);

uint8_t i, j;
int check_if_exist_I2C();
uint8_t portArray[] = {16, 5, 4, 0, 2, 14, 12, 13};
String portMap[] = {"GPIO16", "GPIO5", "GPIO4", "GPIO0", "GPIO2", "GPIO14", "GPIO12", "GPIO13"};

// I2C
Adafruit_BME280 bme;
Adafruit_BMP280 bmp;
Adafruit_ADS1115 adc;
SHT35 sht;

void scanPorts();
bool BME_CNFG = false, BMP_CNFG = false, SHT_CNFG = false, ADC_CNFG = false, MCP = false;
struct I2C
{
    int I2Caddr;
    int scl;
    int sca;
};
struct I2C devices[5];

int configSensors(char *sensorName)
{
    int sensorsInstalled = 0;
    scanPorts();
    String s, sensorArray[DEVICE];
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
    if (sht.begin(0x44))
    {
        sensorArray[sensorsInstalled] = "SHT";
        SHT_CNFG = true;
        sensorsInstalled++;
    }
    setWireBegin(ADC_ADDRESS);
    if (adc.begin(ADC_ADDRESS))
    {
        adc.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
        sensorArray[sensorsInstalled] = "ADC";
        ADC_CNFG = true;
        sensorsInstalled++;
    }
    // pass char *  for parm 1 . NOTE: pass by reference for String is bad! use char *
    for (int j = 0; j < sensorsInstalled; j++)
    {
        if (j > 0)
            strcat(sensorName, "_");
        strcat(sensorName, sensorArray[j].c_str());
    }

    return sensorsInstalled;
}

void readSensor(char *cmd, char *str)
{
    char tmp[512];
    String sensorsData;

    if (BME_CNFG)
    {
        setWireBegin(BMx_ADDRESS);
        sprintf(str, "%x,%f,%f,%f,|,", BME280_ADDRESS, (bme.readTemperature()) * 1.8 + 32,
                bme.readHumidity(), bme.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
    }
    if (BMP_CNFG)
    {
        setWireBegin(BMx_ADDRESS);
        sprintf(str, "%x,%f,%f,|,", BMP280_CHIPID, (bmp.readTemperature()) * 1.8 + 32,
                bmp.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
    }
    if (SHT_CNFG)
    {
        setWireBegin(SHT_ADDRESS);
        if (sht.dataReady())
        {
            sht.read(); // default = true/fast       slow = false
            sprintf(str, "%x,%f,%f,|,", SHT_ADDRESS, sht.getFahrenheit(), sht.getHumidity());
            sensorsData.concat(str);
        }
        else
            strcpy(str, "sht data not ready");
    }
    if (ADC_CNFG)
    {
        setWireBegin(0x48);
        float volts0 = adc.computeVolts(adc.readADC_SingleEnded(0));
        float volts1 = adc.computeVolts(adc.readADC_SingleEnded(1));
        sprintf(str, "%x,%f,%f,|,", ADC_ADDRESS, volts0, volts1);
        Serial.printf("A0/A1 is conected to 3v on esp\n");
        sensorsData.concat(str);
    }
    else
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

                    Serial.print(portArray[i]);
                    Serial.print(",");
                    Serial.println(portArray[j]);
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
            Serial.print("I2C device found at address ");
            if (address < 16)
                Serial.print("0");

            Serial.print(address);
            Serial.print("  0x");
            Serial.println(address, HEX);
#endif
            devices[myCnt].I2Caddr = address;
            devices[myCnt].sca = portArray[i];
            devices[myCnt].scl = portArray[j];
            myCnt++;
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
    for (int j = 0; j < DEVICE; j++)
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
