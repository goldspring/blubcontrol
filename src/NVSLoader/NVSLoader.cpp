#include <M5Unified.h>
#include "NVSLoader.h"
#include <ArduinoJson.h>

void SERIAL_PRINT(const char* msg){
    M5.Display.printf(msg);
    M5.Display.println();
}
void SERIAL_PRINTLN(const char* msg){
    M5.Display.printf(msg);
    M5.Display.println();
}
void SERIAL_PRINTF_LN(const char* msg, const char* s){
    M5.Display.printf(msg, s);
    M5.Display.println();
}
void SERIAL_PRINTF_LN(const char* msg, const char* s, const char* s2){
    M5.Display.printf(msg, s, s2);
    M5.Display.println();
}
static const size_t JSON_FILE_BUFFER_SIZE = 1024;

NVSLoader::NVSLoader(const char* nvsNamespace) : prefs(), NAMESPACE{0} {
    snprintf(NAMESPACE, sizeof(NAMESPACE), "%s", nvsNamespace);
}

NVSLoader::~NVSLoader(void) {
}

bool NVSLoader::begin(const char* path) {
    if (path == nullptr) {
        SERIAL_PRINTLN("path is null");
        return false;
    }
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
        SERIAL_PRINTLN("Failed to mount SPIFFS");
        return false;
    }
    return load(path);
}

bool NVSLoader::get(const char* key, char* buf, size_t size) {
    if (key == nullptr) {
        SERIAL_PRINTLN("key is null");
        return false;
    }
    if (buf == nullptr) {
        SERIAL_PRINTLN("buf is null");
        return false;
    }
    bool result = false;
    if (!this->prefs.begin(this->NAMESPACE, false)) {
        SERIAL_PRINTF_LN("Failed to initialize NVS: %s", this->NAMESPACE);
        return false;
    }
    if (this->prefs.isKey(key)) {
        snprintf(buf, size, "%s", this->prefs.getString(key).c_str());
        result = true;
    }
    this->prefs.end();
    return result;
}

const char* const NVSLoader::getNamespace(void) const {
    return this->NAMESPACE;
}

bool NVSLoader::load(const char* path) {
    if (path == nullptr) {
        SERIAL_PRINTLN("path is null");
        return false;
    }
    if (SPIFFS.exists(path)) {
        char jsonBuffer[JSON_FILE_BUFFER_SIZE] = {0};
        if (!readFile(path, jsonBuffer, sizeof(jsonBuffer))) {
            return false;
        }
        if (!removeFile(path)) {
            return false;
        }
        if (!parse(jsonBuffer, sizeof(jsonBuffer))) {
            return false;
        }
    }
    return true;
}

bool NVSLoader::readFile(const char* path, char* buf, size_t size) {
    if (path == nullptr) {
        SERIAL_PRINTLN("path is null");
        return false;
    }
    File f = SPIFFS.open(path, "r");
    if (!f) {
        SERIAL_PRINTF_LN("Can't open file: %s", path);
        return false;
    }
    size_t fileSize = f.size();
    if (fileSize > size) {
        return false;
    }
    SERIAL_PRINTLN("Reading: ");
    for (size_t p = 0; f.available() && p < size; ++p) {
        buf[p] = f.read();
    }
    f.close();
    return true;
}

bool NVSLoader::removeFile(const char* path) {
    if (path == nullptr) {
        SERIAL_PRINTLN("path is null");
        return false;
    }
    if (!SPIFFS.remove(path)) {
        SERIAL_PRINTF_LN("Failed to remove %s", path);
        return false;
    }
    return true;
}

bool NVSLoader::parse(const char* buf, size_t size) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buf);

    if (error) {
        SERIAL_PRINTLN("Failed to parse the JSON content");
        return false;
    }
    SERIAL_PRINTF_LN("Namespace: %s", this->NAMESPACE);

    if (!this->prefs.begin(this->NAMESPACE, false)) {
        SERIAL_PRINTF_LN("Failed to initialize NVS: %s", this->NAMESPACE);
        return false;
    }
    	
    JsonObject documentRoot = doc.as<JsonObject>();
    for (JsonPair keyValue : documentRoot) {
        prefs.putString(keyValue.key().c_str(), keyValue.value().as<const char*>());
        SERIAL_PRINTF_LN("Writing NVS: key = %s, value = %s", keyValue.key().c_str(), keyValue.value().as<const char*>());
    }

    this->prefs.end();
    return true;
}