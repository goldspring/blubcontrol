#include <string>
#include <sstream>
#include <map>
#include <mbedtls/md5.h>
#include <HTTPClient.h>
#include "MerossClient.h"
#include "WiFiController/WiFiController.h"
#include <ArduinoJson.h>
#include <M5Unified.h>

using namespace std;
time_t MerossClient::prevControl = 0;
int MerossClient::luminanceLevel = 0;

MerossClient::MerossClient(WiFiClient &wificlient, HTTPClient& http, const char* key, const char* url)
    : client(wificlient), httpClient(http), devicekey(key), deviceurl(url){
}

bool MerossClient::ToggleOnOff() {
    int onoff = MerossClient::GetCurrentLightAllData("Appliance.System.All", "togglex", "onoff");
    if (onoff == 0) {
        return TurnOn();
    }
    else{
        return TurnOff();
    }
}

bool MerossClient::GetState() {
    int onoff = MerossClient::GetCurrentLightAllData("Appliance.System.All", "togglex", "onoff");
    return onoff;
}



bool MerossClient::TurnOn() {
    M5.Display.printf("TurnOn");
    JsonDocument payload;
    JsonDocument payload_contents;
    payload_contents["onoff"] = 1;
    payload_contents["channel"] = 0;
    payload["togglex"] = payload_contents;
    string json = buildMessage("SET", "Appliance.Control.ToggleX", payload);
    string response;
    int ret = http_post(json, response);
    return ret == 200;
}

bool MerossClient::TurnOff() {
    M5.Display.printf("TurnOff");
    JsonDocument payload;
    JsonDocument payload_contents;
    payload_contents["onoff"] = 0;
    payload_contents["channel"] = 0;
    payload["togglex"] = payload_contents;
    string json = buildMessage("SET", "Appliance.Control.ToggleX", payload);
    string response;
    int ret = http_post(json, response);
    return ret == 200;
}

bool MerossClient::UpLuminance() {
    return MerossClient::SetLuminance(true);
}
bool MerossClient::DownLuminance() {
    return MerossClient::SetLuminance(false);
}
bool MerossClient::SetRelativeLuminance(int relativeLevel) {
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
                   : (luminance < 1) ? 1
                   : luminance;

    M5.Display.printf("SetRelLumi lvl: %d", luminanceLevel);
    JsonDocument payload;
    JsonDocument payload_contents;
    payload_contents["luminance"] = luminanceLevel;
    payload_contents["channel"] = 0;
    payload_contents["capacity"] = 4;
    payload["light"] = payload_contents;
    string json = buildMessage("SET", "Appliance.Control.Light", payload);
    string response;
    int ret = http_post(json, response);
    return ret == 200;
}

bool MerossClient::SetLuminance(bool up) {
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
                   : (luminance < 1) ? 1
                   : luminance;

    JsonDocument payload;
    JsonDocument payload_contents;
    payload_contents["luminance"] = luminanceLevel;
    payload_contents["channel"] = 0;
    payload_contents["capacity"] = 4;
    payload["light"] = payload_contents;
    string json = buildMessage("SET", "Appliance.Control.Light", payload);
    string response;
    int ret = http_post(json, response);
    return ret == 200;
}

bool MerossClient::SetTemperatureByAltitude(double altitude){
    double horizon = 2700;
	double zenith = 6300;

    /**
     * zenith    -| - - - - - - . - - - - - - -
     *            |       .           .
     * altitude  -|- - 🌞 - - - - - - - -.- - -
     *            |  .                     .
     *            | .                       .
     * horizon   -|.- - - - - - - - - - - - -.-
     */
    int temperature = round((horizon * 1.0) + (altitude * (zenith - horizon)));
    //2700k-6300k
    //0-3600
    int temperaturelevel = round(((temperature - horizon) / (zenith - horizon)) * 100);
	  temperaturelevel = (temperaturelevel > 100) ? 100
                     : (temperaturelevel < 1) ? 1
                     : temperaturelevel;

    return SetTemperature(temperaturelevel);
}
bool MerossClient::SetTemperature(int temperature) {
    /*
    int luminance = GetCurrentLuminance();
    if (luminance == -1) {
        M5.Display.printf("luminanceLevelerror %d\n", luminance);
        return false;
    }
    int currentTemperature = temperature;
    if (currentTemperature == 0) {
        currentTemperature = GetCurrentTemperature();
        if (currentTemperature == -1) {
            M5.Display.printf("currentTemperature error\n");
            return false;
        }
    }*/
    std::map<string, int> tempAndLumi = GetCurrentTemperatureAndLuminance();


    int luminance = tempAndLumi["luminance"];
    if (luminance <= 0) {
        luminance = 1;
    }
    int currentTemperature = temperature;
    if (currentTemperature == 0) {
        currentTemperature = tempAndLumi["temperature"];
        if (currentTemperature <= 0) {
            currentTemperature = 1;
        }
    }
    time_t timet = time(NULL);
    luminanceLevel = (luminance > 100) ? 100
                   : (luminance < 1) ? 1
                   : luminance;
    prevControl = timet;

    M5.Display.printf("SetTemp lvl: %d, temp: %d", luminanceLevel, currentTemperature);
    JsonDocument payload;
    JsonDocument payload_contents;
    payload_contents["luminance"] = luminanceLevel;
    payload_contents["temperature"] = currentTemperature;
    payload_contents["channel"] = 0;
    payload_contents["capacity"] = 6;
    payload["light"] = payload_contents;
    string json = buildMessage("SET", "Appliance.Control.Light", payload);
    string response;
    int ret = http_post(json, response);
    return ret == 200;
}

std::map<string, int> MerossClient::GetCurrentTemperatureAndLuminance() {
    vector<string> v = {"temperature" , "luminance"};
    return MerossClient::GetCurrentLightDataMap("Appliance.Control.Light", v);
}
std::map<string, int> MerossClient::GetCurrentLightDataMap(const char* name_space, const vector<string>& names) {
    JsonDocument payload;
    string json = buildMessage("GET", name_space, payload);
    string response;
    std::map<string, int> retMap;
    int ret = http_post(json, response);
    if (ret != 200){
        return retMap;
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        return retMap;
    }
    for(auto &e :names){
        if (doc.containsKey("payload")
            && doc["payload"].containsKey("light")
            && doc["payload"]["light"].containsKey(e.c_str())) {
            retMap[e] = doc["payload"]["light"][e.c_str()];
        }
    }
    
    return retMap;
}


int MerossClient::GetCurrentTemperature() {
    return MerossClient::GetCurrentLightData("Appliance.Control.Light", "temperature");
}
int MerossClient::GetCurrentLuminance() {
    return MerossClient::GetCurrentLightData("Appliance.Control.Light", "luminance");
}
int MerossClient::GetCurrentLightData(const char* name_space, const char* name) {
    JsonDocument payload;
    string json = buildMessage("GET", name_space, payload);
    string response;
    int ret = http_post(json, response);
    if (ret != 200){
        return ret;
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error
            || !doc.containsKey("payload")
            || !doc["payload"].containsKey("light")
            || !doc["payload"]["light"].containsKey(name)) {
        return -1;
    } else {
        // 正常な場合
        return doc["payload"]["light"][name];
    }
}
int MerossClient::GetCurrentLightAllData(const char* name_space, const char* section, const char* name) {
    JsonDocument payload;
    string json = buildMessage("GET", name_space, payload);
    string response;
    int ret = http_post(json, response);
    if (ret != 200){
        return ret;
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error
            || !doc.containsKey("payload")
            || !doc["payload"].containsKey("all")
            || !doc["payload"]["all"].containsKey("digest")
            || !doc["payload"]["all"]["digest"].containsKey(section)
            || !doc["payload"]["all"]["digest"][section][0].containsKey(name)
            ) {
        return -1;
    } else {
        // 正常な場合
        return doc["payload"]["all"]["digest"][section][0][name];
    }
}
bool MerossClient::SetRgb(int r, int g, int b) {
    int rgb = 0;
    rgb += 65536*r;
    rgb += 256*g;
    rgb += 256*b;

    int luminance = MerossClient::GetCurrentLuminance();
    if (luminance == -1) {
        M5.Display.printf("luminanceLevelerror %d\n", luminance);
        return false;
    }
    time_t timet = time(NULL);
    luminanceLevel = (luminance > 100) ? 100
                   : (luminance < 1) ? 1
                   : luminance;
	prevControl = timet;

    JsonDocument payload;
    JsonDocument payload_contents;
    M5.Display.printf("Setrgb lvl: %d, rgb: %d", luminanceLevel, rgb);
    payload_contents["luminance"] = luminanceLevel;
    payload_contents["rgb"] = rgb;
    payload_contents["channel"] = 0;
    payload_contents["capacity"] = 5;
    payload["light"] = payload_contents;
    string json = buildMessage("SET", "Appliance.Control.Light", payload);
    string response;
    int ret = http_post(json, response);
    return ret == 200;
}

int MerossClient::http_post(string json, string &response) {
    //proxy
    //if (!httpClient.begin(client, "192.168.121.3", 8080, deviceurl.c_str())) {
    if (!httpClient.begin(client, deviceurl.c_str())) {
        return -1;
    }

    httpClient.addHeader("Content-Type", "application/json");
    M5.Display.printf(" s ");
    int ret =  httpClient.POST(json.c_str());
    M5.Display.printf(" r ");
    if (ret != 200) {
        M5.Display.println(response.c_str());
    }
    response.clear();
    response.append(httpClient.getString().c_str());

    httpClient.end();
    return ret;
}

string MerossClient::buildMessage(const char* method, const char* name_space, JsonDocument& payload) {
    uint8_t size = 16;
    char randam[size + 1] = {0};

    for(int i = 0; i < 16; i++){
        randam[i] = genRand(65, 90);
    }

    char uuid[size * 2 + 1] = {0};
    bool ret = make_uuid_md5(randam, uuid, sizeof(uuid));
    if (!ret) {
        return "";
    }
    string messageId(uuid);
    std::transform(messageId.begin(), messageId.end(), messageId.begin(),[](unsigned char c){ return std::tolower(c); });
    time_t timet = time(NULL);
    string timestamp(std::to_string(timet));
    std::ostringstream oss;
    oss << messageId << devicekey << timestamp;
    char signbuf[size * 2 + 1] = {0};
    ret = make_uuid_md5(oss.str().c_str(), signbuf, sizeof(signbuf));
    if (!ret) {
        return "";
    }
    string signature(signbuf);
    std::transform(signature.begin(), signature.end(), signature.begin(),[](unsigned char c){ return std::tolower(c); });
    JsonDocument json;
    JsonDocument hearder_json;
    hearder_json["from"] = deviceurl;
    hearder_json["messageId"] = messageId;  // Example: "122e3e47835fefcd8aaf22d13ce21859"
    hearder_json["method"] =  method; // Example: "GET",
    hearder_json["namespace"] = name_space;  // Example: "Appliance.System.All",
    hearder_json["payloadVersion"] = 1;
    hearder_json["sign"] = signature,  // Example: "b4236ac6fb399e70c3d61e98fcb68b74",
    hearder_json["timestamp"] = timet,
    hearder_json["triggerSrc"] ="Android";
    hearder_json["uuid"] = "2009141640021090830348e1e9339996";

    json["header"] = hearder_json;
    json["payload"] = payload;
    string outputtext;
    serializeJson(json, outputtext);

    return outputtext;
}

bool MerossClient::make_uuid_md5(const char* name, char* uuid, size_t uuid_len) {
    mbedtls_md5_context ctx;
    mbedtls_md5_init(&ctx);
    //mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (const uint8_t*) name, strlen(name));

    uint8_t size = 16;
    uint8_t hash[size];
    mbedtls_md5_finish(&ctx, hash);

    char* ptr = &uuid[0];
    int i;

    for (i = 0; i < size; i++) {
        /* sprintf converts each byte to 2 chars hex string and a null byte, for example
        * 10 => "0A\0".
        *
        * These three chars would be added to the output array starting from
        * the ptr location, for example if ptr is pointing at 0 index then the hex chars
        * "0A\0" would be written as output[0] = '0', output[1] = 'A' and output[2] = '\0'.
        *
        * sprintf returns the number of chars written execluding the null byte, in our case
        * this would be 2. Then we move the ptr location two steps ahead so that the next
        * hex char would be written just after this one and overriding this one's null byte.
        *
        * We don't need to add a terminating null byte because it's already added from 
        * the last hex string. */
        ptr += sprintf(ptr, "%02X", hash[i]);
    }

    mbedtls_md5_free(&ctx);
    return true;
}

int MerossClient::genRand(int min, int max)
{
    static int flag;
    if (flag == 0) {
        srand((unsigned int)time(NULL));
        rand();
        flag = 1;
    }
    int ret = min + (int)(rand()*(max - min + 1.0)/(1.0+RAND_MAX));
    return ret;
}
