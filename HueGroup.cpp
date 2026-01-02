#include "HueGroup.h"
#include "HueLight.h"
#include "HueBridge.h"
#include <ArduinoJson.h>

HueGroup::HueGroup(uint16_t id,
                   const String& name,
                   HueBridge& bridge,
                   const std::vector<uint8_t>& lightIds)
: _id(id), _name(name), _bridge(bridge), _lightIds(lightIds) {}

// --- Getter ---
uint16_t HueGroup::getId() const { return _id; }
const String& HueGroup::getName() const { return _name; }
std::vector<uint8_t> HueGroup::getLightIds() const { return _lightIds; }

bool HueGroup::allOn() const {
    if (_lightIds.empty()) return false;

    for (uint8_t id : _lightIds) {
        HueLight* l = _bridge.getLightById(id);
        if (!l || !l->isOn())
            return false;
    }
    return true;
}

bool HueGroup::anyOn() const {
    for (uint8_t id : _lightIds) {
        HueLight* l = _bridge.getLightById(id);
        if (l && l->isOn())
            return true;
    }
    return false;
}

// --- Setter ---
bool HueGroup::setOn(bool value)     { pending.on = value; return true;}
bool HueGroup::setBri(uint8_t value) { pending.bri = value; return true;}
bool HueGroup::setCT(uint16_t value) { pending.ct = value; return true;}

// --- applyChanges ---
bool HueGroup::applyChanges(HueBridge* bridge) {
    bool hasChanges =
        pending.on.has_value() ||
        pending.bri.has_value() ||
        pending.ct.has_value();

    if (!hasChanges) return true;

    StaticJsonDocument<256> doc;

    if (pending.on)  doc["on"]  = *pending.on;
    if (pending.bri) doc["bri"] = *pending.bri;
    if (pending.ct)  doc["ct"]  = *pending.ct;

    String payload;
    serializeJson(doc, payload);

    if (!bridge->sendGroupState(_id, payload))
        return false;

    // --- lokale HueLights synchronisieren ---
    for (uint8_t id : _lightIds) {
        HueLight* l = _bridge.getLightById(id);
        if (!l) continue;

        if (pending.on)  l->forceOn(*pending.on);
        if (pending.bri) l->forceBri(*pending.bri);
        if (pending.ct)  l->forceCT(*pending.ct);
    }

    clearPending();
    return true;
}


void HueGroup::clearPending() {
    pending.on.reset();
    pending.bri.reset();
    pending.ct.reset();
}
