#ifndef NTPHelper
#define NTPHelper

#include <stdio.h>
#include <WiFiUdp.h>
#include "NTPClient.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "192.168.0.123");

void startNTP()
{
    timeClient.begin();
}

void updateNTP()
{
    timeClient.update();
}

void getTimeNTP(char * timestamp, unsigned int *ms)
{
    unsigned long r;

    r = timeClient.getEpochTimeMs(ms);
    Serial.println("From NTPClient [int]: " );
    Serial.println(r);
    sprintf(timestamp, "%lu", r);
    Serial.println("From NTPClient: ");
    Serial.println(timestamp);
}

#endif NTPHelper
