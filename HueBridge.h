#pragma once
#include <Arduino.h>
#include <vector>
#include <map>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "KinoDevice.h"
#include "HueLight.h"
#include "HueGroup.h"
#include "HueScene.h"
#include "HueSensor.h"

struct SceneLightState {
    bool hasOn  = false;
    bool on     = false;

    bool hasBri = false;
    uint8_t bri = 0;

    bool hasCT  = false;
    uint16_t ct = 0;
};

using SceneLightStates = std::map<uint8_t, SceneLightState>;

class HueLight;

class HueBridge : public KinoDevice {
public:
    const char* deviceType() const override {
        return "huebridge";
    }
    size_t getPropertyCount() const override;
    const KinoPropertyInfo* getPropertyInfo(size_t index) const override;
    KinoError get(const char* property, KinoVariant& out) override;
    KinoError set(const char* property, const KinoVariant& value) override;
    KinoError queryCount(const char* property, uint16_t& out) override;
    KinoError query(const char* property, uint16_t index, KinoVariant &out) override;
    bool needsCommit() override;
    bool commit() override;
    KinoError init() override;    // wie begin, nur andere Semantik


    // Initialisierung
    HueBridge(const IPAddress& ip, const char* user);
    HueBridge(const String& ip, const char* user);
    bool begin();

    // Getter / Setter
    bool setPower(bool onoff);
    bool anyOn();
    
    // Ticker
    KinoError tick();
    int getTickInterval();
    bool setTickInterval(int ms);

    // Lights
    bool readLights();
    HueLight* getLightByName(const char* name);
    HueLight* getLightById(uint8_t id);
    const std::vector<HueLight*>& getLights() const;
    bool sendLightState(uint8_t lightId, const char* jsonPayload);
    int getLightParamCount(const HueLight* l);
    bool getLightParam(const HueLight* l, int paramIndex, char* out, size_t outLen);

    // Groups
    bool readGroups();
    HueGroup* getGroupByName(const char* name);
    const std::vector<HueGroup*>& getGroups() const;
    bool sendGroupState(uint16_t groupId, const char* jsonPayload);
    int getGroupParamCount(const HueGroup* g);
    bool getGroupParam(const HueGroup* g, int paramIndex, char* out, size_t outLen);

    // Scenes
    bool readScenes();
    HueScene* getSceneByName(const char* name);
    //const std::vector<HueScene*>& getScenes() const;
    std::map<uint8_t, bool> getScenePowerStates(const char* sceneId);
    SceneLightStates getSceneLightStates(const char* sceneId);
    bool setScene(const char* sceneName);
    bool saveScene(const char* sceneId, const char* jsonPayload);

    // Sensors
    bool readSensors();
    HueSensor* getSensorByName(const char* name);
    HueSensor* getSensorById(uint16_t sensorId);
    bool setSensorState(uint16_t id, const JsonObject& state);
    bool getSensorParam(const HueSensor* s, int paramIndex, char* out, size_t outLen);

    
private:
    IPAddress _ip;
    char _user[64];
    //WiFiClient& _client;
    std::vector<HueLight*> _lights;
    std::vector<HueGroup*> _groups;
    std::vector<HueScene*> _scenes;
    std::vector<HueSensor*> _sensors;

    // ticker
    int  _tickInterval  = 0;
    unsigned long _lastTick = 0;
    
    // getter und setter - helper    
    bool splitPath(const char* input, char* dev, size_t devLen, char* name, size_t nameLen, char* act, size_t actLen);
    std::vector<String> getLightParams(const HueLight* l);
    std::vector<String> getGroupParams(const HueGroup* g);
    std::vector<String> getSensorParams(const HueSensor* s);

    // read* helper
    void updateOrAddLight(int id, JsonVariant doc);
    void addGroup(int id, JsonVariant doc);
    void addScene(const char* idStr, JsonVariant doc);
    void updateOrAddSensor(int id, JsonVariant doc);
    
    // stream helper
    void EnsureTimeoutBeforeRequest(unsigned long timeout);
    bool findNextKey(WiFiClient& client, char* out, size_t outSize, bool numericOnly);
    size_t _globalDepth;
    // HTTP helper
    bool httpError(WiFiClient& client, const char* cause);
    bool waitForData(WiFiClient& client, uint32_t timeout = 2000);
    bool skipHttpHeader();
    bool httpGET(WiFiClient& client, const char* path);
    bool sendState(const char* path, const char* jsonPayload);

    static const KinoPropertyInfo _properties[];
    
    StaticJsonDocument<128> _filter;
    StaticJsonDocument<1024> _httpJson;
    
};
