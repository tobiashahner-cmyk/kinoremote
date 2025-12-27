#include "HueScene.h"
#include "HueBridge.h"
#include "HueLight.h"
#include <ArduinoJson.h>

HueScene::HueScene(const String& id,
                   const String& name,
                   const std::vector<uint8_t>& lightIds)
: _id(id), _name(name), _lightIds(lightIds) {}

const String& HueScene::getId() const { return _id; }
const String& HueScene::getName() const { return _name; }
const std::vector<uint8_t>& HueScene::getLightIds() const { return _lightIds; }

/*
bool HueScene::setActive(HueBridge& bridge) {

    // 1️⃣ Szene aktivieren (Group 0!)
    StaticJsonDocument<64> doc;
    doc["scene"] = _id;

    String payload;
    serializeJson(doc, payload);

    if (!bridge.sendGroupState(0, payload))
        return false;

    // 2️⃣ Powerstates aus Szene lesen
    std::map<uint8_t, bool> states = bridge.getScenePowerStates(_id);

    // 3️⃣ Workaround für On/Off-Lampen
    for (uint8_t id : _lightIds) {
        HueLight* l = bridge.getLightById(id);
        if (!l) continue;

        if (!l->isDimmable()) {
            auto it = states.find(id);
            if (it == states.end()) continue;

            StaticJsonDocument<32> lightDoc;
            lightDoc["on"] = it->second;

            String lightPayload;
            serializeJson(lightDoc, lightPayload);

            bridge.sendLightState(id, lightPayload);
            l->forceOn(it->second);
        }
    }
    // ziemlich hässlich: Alle Lampen neu einlesen, weil sich die States
    // auf brutale Art geändert haben
    bridge.readLights();
    return true;
}
*/

bool HueScene::setActive(HueBridge* bridge) {

    // 1️⃣ Szene aktivieren (Group 0)
    StaticJsonDocument<64> doc;
    doc["scene"] = _id;

    String payload;
    serializeJson(doc, payload);

    if (!bridge->sendGroupState(0, payload))
        return false;

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

            bridge->sendLightState(id, lightPayload);
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

    String payload;
    serializeJson(doc, payload);

    return bridge->saveScene(_id, payload);
}
