#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_ADS1X15.h>
#include <SHT85.h>

#include <AESLib.h>
#include <CRC.h>
#define SHT85_ADDRESS 0x44

#define NO_SOCKET_AES
#define SEALEVELPRESSURE_HPA (1013.25)
int configSensors(char *sensorName);
void readSensor(char *cmd, char *str);

// I2C
Adafruit_BME280 bme;
Adafruit_BMP280 bmp;
SHT35 sht;
Adafruit_ADS1115 adc;

extern bool BME_CNFG, BMP_CNFG, SHT_CNFG, ADC_CNFG;

int configSensors(char *sensorName)
{
    int sensorsInstalled = 0;
    String s, sensorArray[6];

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
    if (sht.begin())
    {
        sensorArray[sensorsInstalled] = "SHT";
        SHT_CNFG = true;
        sensorsInstalled++;
    }
    Wire.begin(12, 14);
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
        Wire.begin(12, 14);
        float volts0 = 12.5;
        float volts1 = adc.computeVolts(adc.readADC_SingleEnded(1));
        sprintf(str, "0x%x,%f,%f", 0x10,volts0, volts1);
        Serial.printf("A0 is hard code %s\n",str);
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
