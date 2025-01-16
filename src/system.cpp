#include <Arduino.h>

void performSystemTask(char *cmdFromClient);

void performSystemTask(char *cmdFromClient)
{
    if (strstr(cmdFromClient, "BLK"))
    {
        pinMode(LED_BUILTIN, OUTPUT);
        for (int i = 0; i < 10; i++)
        {
            digitalWrite(LED_BUILTIN, HIGH); // Turn the LED on (Note that LOW is the voltage leve
            delay(1000);                     // Wait for a second
            digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off by making the voltage HIGH47
            delay(1000);
        }
    }
    else if (strstr(cmdFromClient, "RST"))
        ESP.reset();
}