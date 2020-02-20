#ifndef NTPHelper
#define NTPHelper

#include <stdio.h>
#include <WiFiUdp.h>
#include "NTPClient.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "192.168.1.126");

void startNTP()
{
    timeClient.begin();
    Serial.println("[NTP] Client started");
}

void updateNTP()
{
    Serial.println("[NTP] Updating client");
    timeClient.updateMs();
}

void getTimeNTP(char * timestamp, unsigned int *ms)
{
    unsigned long r;

    r = timeClient.getEpochTimeMs(ms);
    // Serial.print("From NTPClient [int]: " );
    // Serial.println(r);
    sprintf(timestamp, "%lu", r);
    // Serial.print("From NTPClient: ");
    // Serial.println(timestamp);
}

#endif NTPHelper
