#ifndef globals
#define globals

#include <FS.h>
#ifndef ESP32
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <DNSServer.h>
#ifndef ESP32
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif

#include <ArduinoJson.h>

#define DEBUG 1

bool shouldSaveConfig = false;
bool failedComm=-1;
char server[64];
int port;
#define MAX_NAME_SIZE 10
char NAME[MAX_NAME_SIZE]="S0000";
char ChipId[32];
char device_login[32];
char device_pass[32];
const char sub_dev_modifier[4] = "sub";
const char pub_dev_modifier[4] = "pub";
char prefix[5] = "data";
char _health_channel[] = "_health";
char _rootDomain[64]="data.demo.konkerlabs.net";
int _rootPort=80;
int _httpPort=80;
int _fwPort=80;
char _httpDomain[255]="data.demo.konkerlabs.net";
unsigned long __httpCheckTimout = 5*60000L;
unsigned long __httpLastCheckTS = 0;
unsigned long __wifiTimout=10000L; // 10s to allow auto generated IP (mulitplied by retries)

char *getChipId(){
  return  ChipId;
}

bool interpretHTTPCode(int httpCode){
    if (httpCode > 0 && httpCode<300) { //Check the returning code
        return 1;
    }else{
        if(DEBUG)
        {
            Serial.print("HTTP Code: ");
            Serial.println(String(httpCode));
        }
        return 0;
    }
}

void urldecode2(char *dst, const char *src){
  char a, b;
  while (*src) {
          if ((*src == '%') &&
              ((a = src[1]) && (b = src[2])) &&
              (isxdigit(a) && isxdigit(b))) {
                  if (a >= 'a')
                          a -= 'a'-'A';
                  if (a >= 'A')
                          a -= ('A' - 10);
                  else
                          a -= '0';
                  if (b >= 'a')
                          b -= 'a'-'A';
                  if (b >= 'A')
                          b -= ('A' - 10);
                  else
                          b -= '0';
                  *dst++ = 16*a+b;
                  src+=3;
          } else if (*src == '+') {
                  *dst++ = ' ';
                  src++;
          } else {
                  *dst++ = *src++;
          }
  }
  *dst++ = '\0';
}

String urldecode(String source){
  char chrSource[1024];
  char decodedChrs[1024];
  source.toCharArray(chrSource, 1024);

  urldecode2(decodedChrs,chrSource);
  return (String)decodedChrs;
}


void mydelay(unsigned long _delay) {
        delay(_delay);
        return;
        unsigned long now = millis();
        while ((millis() - now) < _delay) {
                // do nothing
        }
}


#include "../helpers/subChanTuple.h"

#endif
