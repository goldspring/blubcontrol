#pragma once
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>
#include <string>
#include <vector>
#include <AsyncUDP.h>
#include "BulbClient.h"
class WizClient : public BulbClient {
public:
    WizClient(IPAddress ip);
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
    virtual ~WizClient(void){};

protected:
    virtual std::map<std::string, int> GetCurrentTemperatureAndLuminance();
    virtual std::map<std::string, int> GetCurrentLightDataMap(const std::vector<std::string>& names);
    virtual JsonDocument udp_send(const char *sendbuffer);
    template<typename T> T getResultData(T defaultT, JsonDocument& doc, const char* name);
    virtual bool isSuccess(JsonDocument& doc);
private:
    static time_t prevControl;
    static int luminanceLevel;
    IPAddress ipaddr;
};
