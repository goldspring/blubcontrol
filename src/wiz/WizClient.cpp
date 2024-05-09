#include <string>
#include <sstream>
#include <map>
#include <mbedtls/md5.h>
#include <HTTPClient.h>
#include "WizClient.h"
#include "WiFiController/WiFiController.h"
#include <ArduinoJson.h>
#include <M5Unified.h>
#include <AsyncUDP.h>
#include <future>


using namespace std;
time_t WizClient::prevControl = 0;
int WizClient::luminanceLevel = 0;

WizClient::WizClient(IPAddress ip) : ipaddr(ip){
    
}

bool WizClient::ToggleOnOff() {
    bool onoff = GetState();
    if (onoff == 0) {
        return TurnOn();
    }
    else{
        return TurnOff();
    }
}
bool WizClient::GetState() {
    const char on_buffer[]  = "{\"method\":\"getPilot\"}";
    JsonDocument doc = udp_send(on_buffer);
    bool onoff = getResultData<bool>(false, doc, "state");
    return onoff;
}
bool WizClient::TurnOn() {
    M5.Display.printf("TurnOn");
    const char on_buffer[]  = "{\"method\":\"setPilot\",\"params\":{\"state\": 1}}";
    JsonDocument doc = udp_send(on_buffer);
    return isSuccess(doc);
}
bool WizClient::TurnOff() {
    M5.Display.printf("TurnOff");
    const char on_buffer[]  = "{\"method\":\"setPilot\",\"params\":{\"state\": 0}}";
    JsonDocument doc = udp_send(on_buffer);
    return isSuccess(doc);
}

bool WizClient::UpLuminance() {
    return WizClient::SetLuminance(true);
}
bool WizClient::DownLuminance() {
    return WizClient::SetLuminance(false);
}
bool WizClient::SetRelativeLuminance(int relativeLevel) {
    int luminance = luminanceLevel;
    time_t timet = time(NULL);
    if (((unsigned int)timet - (unsigned int)prevControl) > 10) {
        luminance = GetCurrentLuminance();
        if (luminance == -1 ){
            M5.Display.printf("luminanceLevelerror %d\n", luminance);
            return false;
        }
    }
    prevControl = timet;
    luminance += relativeLevel;
    luminanceLevel = (luminance > 100) ? 100
                   : (luminance < 10) ? 10
                   : luminance;


    M5.Display.printf("SetRelLumi lvl: %d", luminanceLevel);
    char buf[100] = {0};
    sprintf(buf, "{\"method\":\"setPilot\",\"params\":{\"state\": 1, \"dimming\":%d}}", luminanceLevel);
    JsonDocument doc = udp_send(buf);
    return isSuccess(doc);
}

bool WizClient::SetLuminance(bool up) {
    int luminance = luminanceLevel;
    time_t timet = time(NULL);
    if (((unsigned int)timet - (unsigned int)prevControl) > 10) {
        luminance = GetCurrentLuminance();
        if (luminance == -1 ){
            M5.Display.printf("luminanceLevelerror %d\n", luminance);
            return false;
        }
    }
    prevControl = timet;
    luminance += (up) ? 5 : -5;
    luminanceLevel = (luminance > 100) ? 100
                   : (luminance < 10) ? 10
                   : luminance;

    char buf[100] = {0};
    sprintf(buf, "{\"method\":\"setPilot\",\"params\":{\"state\": 1, \"dimming\":%d}}", luminanceLevel);
    JsonDocument doc = udp_send(buf);
    return isSuccess(doc);
}

bool WizClient::SetTemperatureByAltitude(double altitude){
    double horizon = 2200;
	double zenith = 6500;

    /**
     * zenith    -| - - - - - - . - - - - - - -
     *            |       .           .
     * altitude  -|- - 🌞 - - - - - - - -.- - -
     *            |  .                     .
     *            | .                       .
     * horizon   -|.- - - - - - - - - - - - -.-
     */
    int temperaturelevel = round((horizon * 1.0) + (altitude * (zenith - horizon)));
    //2700k-6300k
    //0-3600
    temperaturelevel = (temperaturelevel > zenith) ? zenith
                     : (temperaturelevel < horizon) ? horizon
                     : temperaturelevel;

    return SetTemperature(temperaturelevel);
}

bool WizClient::SetTemperature(int temperature) {
    int currentTemperature = temperature;
    if (currentTemperature == 0) {
        currentTemperature = GetCurrentTemperature();
    }
    if (currentTemperature <= 2200) {
        currentTemperature = 2200;
    }
    if (currentTemperature > 6500) {
        currentTemperature = 6500;
    }
    
    time_t timet = time(NULL);
    prevControl = timet;

    M5.Display.printf("SetTemp: %d", currentTemperature);
    char buf[100] = {0};
    sprintf(buf, "{\"method\":\"setPilot\",\"params\":{\"state\": 1, \"temp\":%d}}", currentTemperature);
    JsonDocument doc = udp_send(buf);
    return isSuccess(doc);
}

std::map<string, int> WizClient::GetCurrentTemperatureAndLuminance() {
    vector<string> v = {"temp" , "dimming"};
    return WizClient::GetCurrentLightDataMap(v);
}
std::map<string, int> WizClient::GetCurrentLightDataMap(const vector<string>& names) {
    const char on_buffer[]  = "{\"method\":\"getPilot\"}";
    JsonDocument doc = udp_send(on_buffer);
    std::map<string, int> retMap;
    if (doc["error"]) {
        return retMap;
    }
    for(auto &e :names){
        retMap[e] = getResultData<int>(-1, doc, e.c_str());
    }
    return retMap;
}


int WizClient::GetCurrentTemperature() {
    const char on_buffer[]  = "{\"method\":\"getPilot\"}";
    JsonDocument doc = udp_send(on_buffer);
    return getResultData<int>(-1, doc, "temp");
}

int WizClient::GetCurrentLuminance() {
    const char on_buffer[]  = "{\"method\":\"getPilot\"}";
    JsonDocument doc = udp_send(on_buffer);
    return getResultData<int>(-1, doc, "dimming");
}


template<typename T> T WizClient::getResultData(T defaultT, JsonDocument& doc, const char* name) {
    if (doc["error"]
            || !doc.containsKey("result")
            || !doc["result"].containsKey(name)
            ) {
        return defaultT;
    } else {
        // 正常な場合
        T b = doc["result"][name];
        return b;
    }
}


bool WizClient::SetRgb(int r, int g, int b) {
    M5.Display.printf("Setrgb rgb: %d %d %d %d", luminanceLevel, r, g, b);
    char buf[100] = {0};
    sprintf(buf, "{\"method\":\"setPilot\",\"params\":{\"state\": 1, \"r\":%d, \"g\":%d, \"b\": %d}}", r, g, b);
    JsonDocument doc = udp_send(buf);
    return isSuccess(doc);

}

bool WizClient::isSuccess(JsonDocument& doc) {
    if (doc["error"]
            || !doc.containsKey("result")
            || !doc["result"].containsKey("success")
            ) {
        return false;
    } else {
        // 正常な場合
        bool b = doc["result"]["success"];
        return b;
    }
}

JsonDocument WizClient::udp_send(const char *sendbuffer) {
    AsyncUDP udp;
    std::promise<JsonDocument> pr; // int を受け渡すための promise を作成
    std::future<JsonDocument> ft = pr.get_future(); // これで pr と ft が繋がった
    if(udp.connect(ipaddr, 38899)) {
        udp.onPacket([&pr](AsyncUDPPacket packet) {
            //Serial.write(packet.data(), packet.length());
            //M5.Display.write(packet.data(), packet.length());
            string response((char*)packet.data(), packet.length());
            
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, response);
            doc["error"] = error ? true : false;
            pr.set_value(doc);
        });
        //Send unicast
        udp.print(sendbuffer);
    }
    while (ft.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        // ready が返るまで何か別のことをし続ける
        delay(10);
    }
    return ft.get();
}

