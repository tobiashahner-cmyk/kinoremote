#include "HueGroup.h"
#include "HueLight.h"
#include "HueBridge.h"
#include <ArduinoJson.h>

HueGroup::HueGroup(uint16_t id,
                   const char* name,
                   HueBridge& bridge,
                   const std::vector<uint8_t>& lightIds)
: _id(id), _bridge(bridge), _lightIds(lightIds) 
{ 
  strlcpy(_name, name, sizeof(_name)); 
  _dirty = false;  
}

// --- Getter ---
uint16_t HueGroup::getId() const { return _id; }
const char* HueGroup::getName() const { return _name; }
const std::vector<uint8_t>& HueGroup::getLightIds() const { return _lightIds; }

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

uint16_t HueGroup::getTT() const { return pending.tt.value_or(0);  }

// --- Setter ---
bool HueGroup::setOn(bool value)     { pending.on = value; return true;}
bool HueGroup::setBri(uint8_t value) { pending.bri = value; return true;}
bool HueGroup::setCT(uint16_t value) { pending.ct = value; return true;}
bool HueGroup::setTT(uint16_t value) { pending.tt = (uint16_t)value/100; if (pending.tt == 0) {pending.tt=1;} return true;}

bool HueGroup::isDirty() { return _dirty; }
void HueGroup::setDirty(uint8_t lightId) {
  for (uint8_t id : _lightIds) {
    if (id == lightId) {
      _dirty = true;
      return;
    }
  }
}

void HueGroup::clearDirty() { _dirty = false; }

void HueGroup::updateValues(const char* name, const std::vector<uint8_t> lightIds) {
  // dirty check : neue Lampen dazu?
  bool isdirty = false;
  for (uint8_t lid : lightIds) {
    bool isnew = true;
    for (int i=0; i<_lightIds.size(); i++) {
      if (_lightIds[i] == lid) isnew = false;
    }
    if (isnew) isdirty = true;
  }
  // dirty check 2: alte Lampe entfernt?
  for (uint8_t lid : _lightIds) {
    bool isold = true;
    for (int i=0; i< lightIds.size(); i++) {
      if (lightIds[i] == lid) isold = false;
    }
    if (isold) isdirty = true;
  }
  if (strcmp(_name, name)!=0) {
    strlcpy(_name, name, sizeof(_name));
    _dirty = true;
  }
  if (!isdirty) return;
  _lightIds.clear();
  for (uint8_t lid : lightIds) {
    _lightIds.push_back(lid);
  }
  _dirty = true;
}


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
    if (pending.tt)  doc["transitiontime"] = *pending.tt;

    char payload[128];
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
    _dirty = true;
    return true;
}


void HueGroup::clearPending() {
    pending.on.reset();
    pending.bri.reset();
    pending.ct.reset();
    pending.tt.reset();
}
