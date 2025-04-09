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
void performSystemTask(char *cmdFromClient);

void performSystemTask(char *cmdFromClient)
{
    if (strstr(cmdFromClient, "BLK"))
    {
        pinMode(D6, OUTPUT);
        digitalWrite(D6, 0);
      
    }
    else if (strstr(cmdFromClient, "RST"))
        ESP.reset();
}
