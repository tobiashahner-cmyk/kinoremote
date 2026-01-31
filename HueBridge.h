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

    HueBridge(WiFiClient& wfc, const IPAddress& ip, const String& user);
    HueBridge(WiFiClient& wfc, const String& ip, const String& user);

    bool begin();
    bool readLights();

    KinoError tick();
    int getTickInterval();
    bool setTickInterval(int ms);

    HueLight* getLightByName(const String& name);
    HueLight* getLightById(uint8_t id);
    const std::vector<HueLight*>& getLights() const;

    bool readGroups();

    HueGroup* getGroupByName(const String& name);
    const std::vector<HueGroup*>& getGroups() const;

    bool readScenes();
    HueScene* getSceneByName(const String& name);
    //const std::vector<HueScene*>& getScenes() const;
    bool setScene(const String& sceneName);

    bool readSensors();
    HueSensor* getSensorByName(const String& name);
    HueSensor* getSensorById(uint16_t sensorId);
    bool setSensorState(uint16_t id, const JsonObject& state);

    bool setPower(bool onoff);
    bool anyOn();
    

    // --- HTTP PUT für Light State ---
    bool sendLightState(uint8_t lightId, const String& jsonPayload);
    bool sendGroupState(uint16_t groupId, const String& jsonPayload);
    bool sendState(const String& path, const String& jsonPayload);
    bool saveScene(const String& sceneId, const String& jsonPayload);
    std::map<uint8_t, bool> getScenePowerStates(const String& sceneId);
    SceneLightStates getSceneLightStates(const String& sceneId);

    
private:
    IPAddress _ip;
    String _user;
    WiFiClient& _client;

    bool httpError(const char* cause);
    
    std::vector<HueLight*> _lights;
    std::vector<HueGroup*> _groups;
    std::vector<HueScene*> _scenes;
    std::vector<HueSensor*> _sensors;
    bool splitPath(const char* input, char* dev, size_t devLen, char* name, size_t nameLen, char* act, size_t actLen);

    std::vector<String> getLightParams(const HueLight* l);
    std::vector<String> getGroupParams(const HueGroup* g);
    std::vector<String> getSensorParams(const HueSensor* s);

    
    int  _tickInterval  = 0;
    unsigned long _lastTick = 0;
    void EnsureTimeoutBeforeRequest(unsigned long timeout);
    bool httpGET(const String& path);
    bool waitForData(uint32_t timeout = 2000);
    bool skipHttpHeader();

    static const KinoPropertyInfo _properties[];
};
