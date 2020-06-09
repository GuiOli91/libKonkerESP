
#ifndef firmwareUpdate
#define firmwareUpdate

#include "../helpers/globals.h"
#ifndef ESP32
#include <ESP8266HTTPClient.h>
#else
#include <HTTPClient.h>
#endif

#ifndef ESP32
#include <ESP8266httpUpdate.h>
#else
#include <ESP32httpUpdate.h>
#endif

#include "../helpers/jsonhelper.h"
#include "../helpers/NTPHelper.h"
#include "../helpers/globals.h"
#include "../mqtt/pubsubMQTT.h"

#if !defined(ESP8266) || defined(ESP32)
#include <functional>
#define UPDATE_SUCCESS_CALLBACK_SIGNATURE std::function<void(char[16])> succes_update_callback
#else
#define UPDATE_SUCCESS_CALLBACK_SIGNATURE void (*succes_update_callback)(char[16], char[16], unsigned int)
#endif

unsigned long _last_time_update_check=0;

#ifndef ESP32
class ESPHTTPKonkerUpdate: public ESP8266HTTPUpdate{
  public:
  t_httpUpdate_return update(const String& host, uint16_t port, const String& uri = "/",
                               const String& currentVersion = ""){
    t_httpUpdate_return ret;
    HTTPClient http;
    http.begin(host, port, uri);

    Serial.print("Fetching binary at: ");
    Serial.println(host + ":" + String(port) + uri);

    Serial.println("Authorizing...");
    http.setAuthorization(device_login, device_pass);
    Serial.println("Authorization successfull");
    // [MJ] Função recebe e checa MD5 (vem no header)
    ret = ESP8266HTTPUpdate::handleUpdate(http, currentVersion, false);

    Serial.println("Return code: " + ESP8266HTTPUpdate::getLastErrorString());

    return ret;
  }
};
#else
class ESPHTTPKonkerUpdate: public ESP32HTTPUpdate{
  public:
  t_httpUpdate_return update(const String& host, uint16_t port, const String& uri = "/",
                               const String& currentVersion = ""){
    HTTPClient http;
    http.begin(host, port, uri);

    Serial.println("Authorizing..");
    http.setAuthorization(device_login, device_pass);
    return ESP32HTTPUpdate::handleUpdate(http, currentVersion, false);
  }
};
#endif

void getVersion(String strPayload, char *version)
{
    if(parse_JSON_item(strPayload,"version",version))
    {
        Serial.println("New version = " + String(version));
    }
    else
    {
        strcpy(version,"");
        Serial.println("Failed to parse version");
    }
}

void getCurrentTime(char * timestamp, unsigned int * ms)
{
    getTimeNTP(timestamp, ms);
    Serial.print("Changing state at: ");
    Serial.print(timestamp);
    Serial.println(*ms);
}

void updateSucessCallBack(char *version, char *ts1, unsigned int ms1)
{
    char ts2[16] = {"\0"};
    unsigned int ms2;
    bool subCode=0;

    Serial.println("[Update] Update ok, sending confirmation.");
    getCurrentTime(ts2, &ms2);

    String fwUpdateURL= "http://" + String(_httpDomain) + String (":") + String(_httpPort) + String("/registry-data/firmware/") + String(device_login);
    HTTPClient http;  //Declare an object of class HTTPClient
    http.begin(fwUpdateURL);  //Specify request destination
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.setAuthorization(device_login, device_pass);

    String smsg=String("{\"version\": \"" + String(version) + "\",\"status\":\"UPDATED\"}");
    int httpCode = http.sendRequest("PUT", String(smsg));


    Serial.println("Confirmantion send: " + fwUpdateURL+  + "; Body: " + smsg + "; httpcode: " + String(httpCode));
    // Serial.print(">");

    String strPayload = http.getString();
    Serial.print("[Update callback] response  body: ");
    Serial.println(strPayload);
    http.end();   //Close connection

    subCode=interpretHTTPCode(httpCode);

    if (!subCode){
        Serial.println("[Update callback] failed");
    }else{
        Serial.println("[Update callback] sucess");
    }
    // [MJ] Send timestamps to platform via MQTT
    const int capacity = 1024; // JSON_OBJECT_SIZE(200);
    StaticJsonDocument<capacity> jsonMSG;
    char bufferJson[256];

    jsonMSG["deviceId"] = NAME;
    jsonMSG["ts1"] = ts1;
    jsonMSG["ms1"] = ms1;
    jsonMSG["ts2"] = ts2;
    jsonMSG["ms2"] = ms2;
    serializeJson(jsonMSG, bufferJson);

    pubMQTT("timestamps", bufferJson);
}

bool hasUpdate(char *rootDomain,int rootPort, char *version, char *ts, unsigned int *ms){
  bool subCode=0;
  Serial.println("[Update] Checking for updates...");
  char buffer[100];
  char bffPort[6];
  String sPort=(String)rootPort;
  sPort.toCharArray(bffPort, 6);

  if (String(rootDomain).indexOf("http://", 0)>0){
    strcpy (buffer,rootDomain);
    strcat (buffer,":");
    strcat (buffer,bffPort);
    strcat (buffer, "/registry-data"); // [MJ] Apenas para plataforma rodando localmente
    strcat (buffer,"/firmware/");
    strcat (buffer,device_login);
  }else{
    strcpy (buffer,"http://");
    strcat (buffer,rootDomain);
    strcat (buffer,":");
    strcat (buffer,bffPort);
    strcat (buffer, "/registry-data"); // [MJ] Apenas para plataforma rodando localmente
    strcat (buffer,"/firmware/");
    strcat (buffer,device_login);
  }

  HTTPClient http;  //Declare an object of class HTTPClient
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(2000);
  http.setAuthorization(device_login, device_pass);
  http.begin((String)buffer);  //Specify request destination
  int httpCode = http.GET();

  Serial.println("Checking update at: " + String(buffer));

  subCode=interpretHTTPCode(httpCode);

  if (!subCode){
    Serial.println("[No Update] request failed");
    Serial.println("");
    strcpy(version,""); // [MJ] Se updatee falha, string da versão preenchida com vazio
  }else{
    Serial.println("[Update] request sucess");
    Serial.println("");

    String strPayload = http.getString();
    Serial.println("strPayload=" + strPayload);
    int playloadSize=http.getSize();
    if (strPayload!="[]"){
      getVersion(strPayload,version);
    }
    getCurrentTime(ts, ms); // [MJ] Muda estado para UPDATING na plataforma (automaticamente)
  }
  http.end();   //Close connection

  return subCode;
}

void checkForUpdates(char *rootDomain,int rootPort, char *expectedVersion, UPDATE_SUCCESS_CALLBACK_SIGNATURE){
    if (_last_time_update_check!=0){
        //throtle this call at maximum 1 per minute
        if ((millis()-_last_time_update_check) < 6500){
            //Serial.println("checkForUpdates maximum calls is 1/minute. Please wait more to call again");
            return;
        }
    }

    _last_time_update_check = millis();

    char version[16];
    char ts[16] = {"\0"};
    unsigned int ms;
    if (hasUpdate(rootDomain, rootPort, version, ts, &ms)){
        if(String(version).indexOf(String(expectedVersion))>=0 || String(version)==""){
            Serial.println("UPDATING....");
            ESPHTTPKonkerUpdate ESPhttpKonkerUpdate;
            ESPhttpKonkerUpdate.rebootOnUpdate(false);
            t_httpUpdate_return ret = ESPhttpKonkerUpdate.update(String(rootDomain), rootPort, String("/registry-data/firmware/") + String(device_login) +String("/binary"));
            switch(ret) {
                case HTTP_UPDATE_FAILED:
                    Serial.println("[Update] FW update failed.");
                    break;
                case HTTP_UPDATE_NO_UPDATES:
                    Serial.println("[Update] No update.");
                    break;
                case HTTP_UPDATE_OK:
                    // Serial.println("[Update] Not sending confirmation!!! D:D:D:D:");
                    updateSucessCallBack(version, ts, ms);
                    ESP.restart();
                    break;
            }
            Serial.println("");
        }
    }
}

void checkForUpdates(){
  char expectedVersion[16]="";

  checkForUpdates(_httpDomain, _httpPort, expectedVersion, updateSucessCallBack);
}

#endif
