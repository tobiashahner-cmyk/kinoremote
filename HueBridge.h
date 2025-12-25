#pragma once
#include <Arduino.h>
#include <vector>
#include <map>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
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

class HueBridge {
public:
    HueBridge(const IPAddress& ip, const String& user);
    HueBridge(const String& ip, const String& user);

    bool begin();
    bool readLights();

    bool tick();
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
    const std::vector<HueScene*>& getScenes() const;
    bool setScene(const String& sceneName);

    bool readSensors();
    HueSensor* getSensorByName(const String& name);
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
    WiFiClient _client;
    std::vector<HueLight*> _lights;
    std::vector<HueGroup*> _groups;
    std::vector<HueScene*> _scenes;
    std::vector<HueSensor*> _sensors;

    
    int  _tickInterval  = 10000;
    unsigned long _lastTick = 0;

    bool httpGET(const String& path);
    bool waitForData(uint32_t timeout = 2000);
    bool skipHttpHeader();
};
