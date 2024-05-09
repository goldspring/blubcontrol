#pragma once
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>
#include <string>
#include <vector>
#include "BulbClient.h"
class MerossClient : public BulbClient {
public:
    MerossClient(WiFiClient &client, HTTPClient& http, const char* key, const char* url);
    virtual bool ToggleOnOff();
    /**/virtual bool GetState();
    /**/virtual bool TurnOn();
    /**/virtual bool TurnOff();
    
    virtual bool UpLuminance();
    virtual bool DownLuminance();
    virtual bool SetLuminance(bool up);
    /**/virtual bool SetRelativeLuminance(int relativeLevel);
    /**/virtual bool SetRgb(int r, int g, int b);
    /**/virtual bool SetTemperatureByAltitude(double altitude);
    virtual int GetCurrentLuminance();
    virtual bool SetTemperature(int temperature);
    virtual int GetCurrentTemperature();
    virtual int GetCurrentLightData(const char* name_space, const char* name);
    virtual int GetCurrentLightAllData(const char* name_space, const char* section, const char* name);
    virtual ~MerossClient(void){};

protected:
    virtual std::map<std::string, int> GetCurrentTemperatureAndLuminance();
    virtual std::map<std::string, int> GetCurrentLightDataMap(const char* name_space, const std::vector<std::string>& names);
    virtual int http_post(std::string json, std::string &response);
    virtual std::string buildMessage(const char* method, const char* name_space, JsonDocument& payload);
    virtual bool make_uuid_md5(const char* name, char* uuid, size_t uuid_len);
    virtual int genRand(int min, int max);
private:
    static time_t prevControl;
    static int luminanceLevel;
    WiFiClient& client;
    HTTPClient& httpClient;
    std::string devicekey;
    std::string deviceurl;
};
