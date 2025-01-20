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
#define SHT85_ADDRESS 0x44

#define NO_SOCKET_AES
#define SEALEVELPRESSURE_HPA (1013.25)
int myCnt = 0;
int configSensors(char *sensorName);
void readSensor(char *cmd, char *str);
int getI2C_pins(int addr);

uint8_t i, j;
int check_if_exist_I2C();
uint8_t portArray[] = {16, 5, 4, 0, 2, 14, 12, 13};
String portMap[] = {"GPIO16", "GPIO5", "GPIO4", "GPIO0", "GPIO2", "GPIO14", "GPIO12", "GPIO13"};

// I2C
Adafruit_BME280 bme;
Adafruit_BMP280 bmp;
SHT35 sht;
Adafruit_ADS1115 adc;
void scanPorts();

extern bool BME_CNFG, BMP_CNFG, SHT_CNFG, ADC_CNFG;

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
    getI2C_pins(0x76);
    if (bme.begin(0x76))
    {
        sensorArray[sensorsInstalled] = "BME";
        BME_CNFG = true;
        sensorsInstalled++;
    }
    if (bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID))
    {
        sensorArray[sensorsInstalled] = "BMP";
        Serial.println(bmp.readTemperature());
        BMP_CNFG = true;
        sensorsInstalled++;
    }
    getI2C_pins(0x44);
    if (sht.begin())
    {
        sensorArray[sensorsInstalled] = "SHT";
        SHT_CNFG = true;
        sensorsInstalled++;
    }
    getI2C_pins(0x48);
    if (adc.begin(0x48))
    {
        adc.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
        sensorArray[sensorsInstalled] = "ADC";
        ADC_CNFG = true;
        sensorsInstalled++;
        float volts1 = adc.computeVolts(adc.readADC_SingleEnded(1));
        Serial.printf("adc %f\n", volts1);
    }
    // create char *  for parm 1 . NOTE: pass by reference for String is bad! use char *
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
    //
    char tmp[512];
    String sensorsData;

    if (BME_CNFG)
    {
        sprintf(str, "0x%x,%f,%f,%f,", bme.sensorID(), (bme.readTemperature()) * 1.8 + 32,
                bme.readHumidity(), bme.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
    }
    if (BMP_CNFG)
    {
        sprintf(str, "0x%x,%f,%f,", bmp.sensorID(), (bmp.readTemperature()) * 1.8 + 32,
                bmp.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
    }
    if (SHT_CNFG)
    {
        if (sht.dataReady())
        {
            sht.read(); // default = true/fast       slow = false
            sprintf(str, "0x%x,%f,%f,", sht.getType(), sht.getFahrenheit(), sht.getHumidity());
            sensorsData.concat(str);
        }
        else
            strcpy(str, "sht data not ready");
    }
    if (ADC_CNFG)
    {
        getI2C_pins(0x48);
        float volts0 = 12.5;
        float volts1 = adc.computeVolts(adc.readADC_SingleEnded(1));
        sprintf(str, "0x%x,%f,%f", 0x10, volts0, volts1);
        Serial.printf("A0 is hard code %s\n", str);
        sensorsData.concat(str);
    }
    else
        strcpy(str, "0");

    sensorsData = sensorsData.substring(0, sensorsData.length() - 1); // remove the last ","
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
                //  Serial.print("Scanning (SDA : SCL) - " + portMap[i] + " : " + portMap[j] + " - ");
                Wire.begin(portArray[i], portArray[j]);
                if (check_if_exist_I2C())
                {

                    Serial.print(portArray[i]);
                    Serial.print(",");
                    Serial.println(portArray[j]);
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
            Serial.print("I2C device found at address ");
            if (address < 16)
                Serial.print("0");
            Serial.print(address);
            Serial.print("  0x");
            Serial.println(address, HEX);
            devices[myCnt].I2Caddr = address;
            devices[myCnt].sca = portArray[i];
            devices[myCnt].scl = portArray[j];
            myCnt++;
            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknow error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    return nDevices;
}
int getI2C_pins(int addr)
{
    for (int j = 0; j < DEVICE; j++)
    {
        if (addr == devices[j].I2Caddr)
        {
            Wire.begin(devices[j].sca, devices[j].scl);
            return 1;
        }
    }
    return 0;
}
