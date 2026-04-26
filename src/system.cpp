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
//void convert_ivs_to_hex();
extern byte aes_key[N_BLOCK];
extern byte aes_iv[N_BLOCK];

extern byte aes_iv_copy[N_BLOCK];

extern char results[];
extern char cleartext[];
extern byte new_aes_iv[16];
extern char ciphertext[4096];
int length = -1;

String readLittle(char *fileName);
int writeLittle(char *fileName, const char *message);

uint16_t encrypt_to_ciphertext(char *msg, byte iv[], byte aes[]);
void decrypt_to_cleartext(char *msg, uint16_t msgLen, byte iv[], byte key[], char *cleartext);
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

        // if (length > 0)
        //      strcpy(results, "next boot aes key will be updated");
        //  else
        //      strcpy(results, "failed to update");
    }
}
// void convert_ivs_to_hex() {
//     // 1. Example IVs (16 bytes each)
//     //0102030405060708090a0b0c0d0e0f10
//     unsigned char iv[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

//     unsigned char iv_copy[16];
//     memcpy(iv_copy, iv, 16);

//     // 3. Convert to Hex String (16 bytes * 2 chars + null terminator)
//     char hexAscii[33]; 
//     for (int i = 0; i < 32; i++) {
//         sprintf(hexAscii + (i * 2), "%02x", iv_copy[i]);
//     }
//     hexAscii[32] = '\0';

//     printf("Hex String: %s\n", hexAscii);
// }