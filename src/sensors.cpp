#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <AESLib.h>
#include <CRC.h>
#include <SHT85.h>
#define SHT85_ADDRESS 0x44

#define NO_SOCKET_AES
#define SEALEVELPRESSURE_HPA (1013.25)
int configSensors(char *sensorName);
void readSensor(char *cmd, char *str);

Adafruit_BME280 bme; // I2C
Adafruit_BMP280 bmp; // I2C
SHT85 sht;

extern bool BME_CNFG, BMP_CNFG, SHT_CNFG;

int configSensors(char *sensorName)
{
    int sensorsInstalled = 0;
    while (1)
    {
        if (!BME_CNFG && bme.begin(0x76))
        {
            Serial.println("BME280 detected ");
            strcpy(sensorName, "BME280");
            BME_CNFG = true;
            sensorsInstalled++;
            continue;
        }
        if (!BMP_CNFG && bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID))
        {
            Serial.println("BMP280 detected ");
            strcpy(sensorName, "BMP280");
            BMP_CNFG = true;
            Serial.println(bmp.readTemperature());
            sensorsInstalled++;
            continue;
        }
        if (!SHT_CNFG && sht.begin(0x44))
        {
            Serial.println("SHT35 detected ");
            strcpy(sensorName, "SHT35");
            SHT_CNFG = true;
            sensorsInstalled++;
            continue;
        }
        else
            break;
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
        sprintf(str, "0x%x,%f,%f,%f", bme.sensorID(), (bme.readTemperature()) * 1.8 + 32,
                bme.readHumidity(), bme.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
    }
    if (BMP_CNFG)
    {
        sprintf(str, "0x%x,%f,%f", bmp.sensorID(), (bmp.readTemperature()) * 1.8 + 32,
                bmp.readAltitude(SEALEVELPRESSURE_HPA));
        sensorsData.concat(str);
    }
    if (SHT_CNFG)
    {
        if (sht.dataReady())
        {
            sht.read(); // default = true/fast       slow = false
            sprintf(str, "%x,%f,%f", sht.GetSerialNumber(),sht.getFahrenheit(), sht.getHumidity());
            Serial.println(str);
            sensorsData.concat(str);
        }
        else
            strcpy(str, "sht data not ready");
    }
    else
    {
        strcpy(str, "0");
        // Serial.printf("No devive found for %s on %s\n", cmd, Buf + 4);
    }
    strcpy(tmp, sensorsData.c_str());
    uint8_t *data = (uint8_t *)&tmp[0]; // ptr to 1st char in str
    uint32_t t = calcCRC32(data, strlen(tmp));
    bzero(tmp, 512);
    sprintf(tmp, "%x:%s", t, sensorsData.c_str());
#ifndef NO_SOCKET_AES
    encrypt_stub(str, encrypt_string);
    Serial.printf("in server aes bmp %s", encrypt_string);
#else
    strcpy(str, tmp);
    Serial.printf("sent to client %s\n", str);
#endif
}
