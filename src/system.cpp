/**
 * @file system.cpp
 * @brief Contains system-related functionality for the ESP8266 server project.
 *
 * This file implements a function to perform specific system tasks based on
 * commands received from a client.
 */

/**
 * @brief Executes system tasks based on the provided command.
 *
 * This function processes a command string and performs specific actions:
 * - If the command contains "BLK", it blinks the built-in LED 10 times with a 1-second interval.
 * - If the command contains "RST", it resets the ESP8266 device.
 *
 * @param cmdFromClient A null-terminated string containing the command from the client.
 *
 * @note Ensure that the command string is properly null-terminated to avoid undefined behavior.
 * @note The "RST" command uses `ESP.reset()` to reset the device, which may cause loss of unsaved data.
 */
#include <Arduino.h>
#include <AESLib.h>
#include <ESP8266WiFi.h>
void blkMe();
#define INPUT_BUFFER_LIMIT 2048
int beginWIFI(String sensorName);
//void convert_ivs_to_hex();
extern byte aes_key[N_BLOCK];
extern byte aes_iv[N_BLOCK];

extern byte aes_iv_copy[N_BLOCK];

extern char cleartext[INPUT_BUFFER_LIMIT];
extern byte new_aes_iv[16];
extern char ciphertext[INPUT_BUFFER_LIMIT*2];
int length = -1;

String readLittle(char *fileName);
int writeLittle(char *fileName, const char *message);

uint16_t encrypt_to_ciphertext(char *msg, byte iv[], byte aes[]);
void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[], byte key[], char *cleartext);
void performSystemTask(char *cmdFromClient)
{

    Serial.printf("cmd recieved:%s\n",cmdFromClient);

    if (strstr(cmdFromClient, "BLK"))
        blkMe();
    else if (strstr(cmdFromClient, "RST"))
        ESP.reset();
    // else if (strstr(cmdFromClient, "NEW"))
    //     int i = 0;
    else if (strstr(cmdFromClient, "CHG")) // change aes_key
    {
        // re-encrypting stored WiFi credentials with a newly generated AES key and saving them to the filesystem .
        // todo need to test wifi connection and update pc
        //convert_ivs_to_hex();
        byte new_aes_key[N_BLOCK];
        // byte new_aes_iv[N_BLOCK];

        String ssid_psw_aes = readLittle((char *)"/ssid_pass_aes.txt"); // read original encrypted ssid and password
        memcpy(aes_iv_copy, aes_iv, sizeof(aes_iv));
        decrypt_to_cleartext((char *)ssid_psw_aes.c_str(), strlen(ssid_psw_aes.c_str()), aes_iv_copy, aes_key, cleartext);

        AESLib aesLib;
        aesLib.gen_iv(new_aes_key); // iv ng  key works  WTF ????
        memcpy(aes_iv_copy, aes_iv, sizeof(aes_iv));
        length = encrypt_to_ciphertext(cleartext, aes_iv_copy, new_aes_key); // now encrypt ssid and password with new key

        // writeLittle((char *)"/new_ssid_pass_aes.txt", ciphertext); // save

        String data = readLittle((char *)"/new_ssid_pass_aes.txt"); // round robin verification
        memcpy(aes_iv_copy, aes_iv, sizeof(aes_iv));
        decrypt_to_cleartext((char *)data.c_str(), strlen(data.c_str()), aes_iv_copy, new_aes_key, cleartext);
        Serial.printf("ciphertext %s \n", ciphertext);
        for (int i = 0; i < 16; i++)
        {
            Serial.printf("%x,", new_aes_key[i]);
        }
        Serial.println();

    }
}
void blkMe()
{
    pinMode(LED_BUILTIN, OUTPUT);

    for (int i = 0; i < 5; i++)
    {
        digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off
        delay(500);                      // Wait for a second
        digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on
        delay(500);
    }
    digitalWrite(LED_BUILTIN, HIGH);
}