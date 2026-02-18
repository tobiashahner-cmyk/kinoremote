#include "HueScene.h"
#include "HueBridge.h"
#include "HueLight.h"
#include <ArduinoJson.h>

HueScene::HueScene(const char* id, const char* name, const std::vector<uint8_t>& lightIds)
: _lightIds(lightIds) { 
  strlcpy(_name, name, sizeof(_name));
  strlcpy(_id, id, sizeof(_id));
}

const char* HueScene::getId() const { return _id; }
const char* HueScene::getName() const { return _name; }



const uint16_t HueScene::getTT() const { return _tt; }
const std::vector<uint8_t>& HueScene::getLightIds() const { return _lightIds; }

bool HueScene::setTT(uint16_t value) { _tt = (uint16_t)value/100; if (_tt==0) _tt=1; return true;}

bool HueScene::setActive(HueBridge* bridge) {

    // 1️⃣ Szene aktivieren (Group 0)
    StaticJsonDocument<64> doc;
    doc["scene"] = _id;
    doc["transitiontime"] = _tt;

    char payload[64];
    serializeJson(doc, payload);

    if (!bridge->sendGroupState(0, payload))
        return false;

    // reset transition time
    _tt = 4;  // Standardwert in der Bridge, wenn nichts Anderes angegeben

    // 2️⃣ Gewünschte Zustände aus der Szene lesen
    SceneLightStates states =
        bridge->getSceneLightStates(_id);

    // 3️⃣ Lokale HueLights synchronisieren
    for (uint8_t id : _lightIds) {
        HueLight* l = bridge->getLightById(id);
        if (!l) continue;

        auto it = states.find(id);
        if (it == states.end()) continue;

        const SceneLightState& s = it->second;

        // Workaround für On/Off-Lampen
        if (!l->isDimmable() && s.hasOn) {
            StaticJsonDocument<32> lightDoc;
            lightDoc["on"] = s.on;

            String lightPayload;
            serializeJson(lightDoc, lightPayload);

            bridge->sendLightState(id, lightPayload.c_str());
        }

        // Lokalen Cache synchronisieren
        if (s.hasOn)  l->forceOn(s.on);
        if (s.hasBri) l->forceBri(s.bri);
        if (s.hasCT)  l->forceCT(s.ct);
    }

    return true;
}



bool HueScene::captureLightStates(HueBridge* bridge) {
    StaticJsonDocument<64> doc;
    doc["storelightstate"] = true;

    char payload[48];
    serializeJson(doc, payload);

    return bridge->saveScene(_id, payload);
}
