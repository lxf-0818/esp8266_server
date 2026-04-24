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

int beginWIFI(String sensorName);
extern byte aes_iv_copy[N_BLOCK];
extern byte aes_iv[N_BLOCK];
extern char results[];
uint16_t encrypt_to_ciphertext(char *msg, byte iv[], byte aes[]);

extern byte new_aes_iv[16];
extern char ciphertext[4096];

void performSystemTask(char *cmdFromClient)
{
    if (strstr(cmdFromClient, "BLK"))
        digitalWrite(D6, 0);
    else if (strstr(cmdFromClient, "RST"))
        ESP.reset();
    else if (strstr(cmdFromClient, "NEW"))
        int i = 0;
    else if (strstr(cmdFromClient, "CHG")) // change aes_key
    {
        // beginWIFI(""); // verify

        AESLib aesLib;
        aesLib.gen_iv(new_aes_iv);
#ifdef DEBUG
        for (int j = 0; j < 16; j++)
            Serial.printf("%d , ", new_aes_iv[j]);
        Serial.println();
#endif
        char str[] = "NETGEAR37-2:grandcurtain880";
        char aes_encrypt[512];
        memcpy(aes_iv_copy, aes_iv, sizeof(aes_iv));
        int length = encrypt_to_ciphertext(str, aes_iv_copy, new_aes_iv);
        strncpy(aes_encrypt, ciphertext, length + 1);
        Serial.printf("clear text      %s\n", str);
        Serial.printf("encrypt string: %s\n", ciphertext);
       // strcpy(results, "foo"); 
    }
}