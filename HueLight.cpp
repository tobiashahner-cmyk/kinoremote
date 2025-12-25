#include "HueLight.h"
#include "HueBridge.h"
#include <ArduinoJson.h>

HueLight::HueLight(uint8_t id,
                   const String& name,
                   bool on,
                   uint8_t bri,
                   bool hasXY,
                   float x,
                   float y,
                   bool hasCT,
                   uint16_t ct,
                   bool hasBri)
: _id(id),
  _name(name),
  _on(on),
  _bri(bri),
  _hasBri(hasBri),
  _hasXY(hasXY),
  _x(x),
  _y(y),
  _hasCT(hasCT),
  _ct(ct) {}

// --- Getter ---
uint8_t HueLight::getId() const { return _id; }
const String& HueLight::getName() const { return _name; }

bool HueLight::isOn() const { return _on; }
uint8_t HueLight::getBrightness() const { return _bri; }
bool HueLight::isDimmable() const { return _hasBri; }

bool HueLight::hasXYColor() const { return _hasXY; }
bool HueLight::hasCTColor() const { return _hasCT; }

float HueLight::getX() const { return _x; }
float HueLight::getY() const { return _y; }
uint16_t HueLight::getCT() const { return _ct; }

// --- Setter ---
bool HueLight::setOn(bool value)      { pending.on = value; return true;}
bool HueLight::setBri(uint8_t value)  { if(_hasBri) pending.bri = value; return true;}
bool HueLight::setCT(uint16_t value)  { if(_hasCT) { pending.ct = value;  return true; } return false;}
bool HueLight::setXY(float x, float y){ if(_hasXY) { pending.xy = std::make_pair(x, y);  return true;}  return false;}
bool HueLight::setRGB(uint8_t r, uint8_t g, uint8_t b) { return true; /* TODO: umrechner zu XY, dann setXY() */}

// --- spezielle Setter für direkten Zugriff ohne "pending"
void HueLight::forceOn(bool value)              { _on = value; }
void HueLight::forceBri(uint8_t value)          { if (_hasBri) _bri = value; }
void HueLight::forceCT(uint16_t value)          { if (_hasCT)  _ct = value; }

// --- applyChanges ---
bool HueLight::applyChanges(HueBridge& bridge) {
    // Prüfen, ob überhaupt Änderungen vorliegen
    bool hasChanges = pending.on.has_value() || 
                      pending.bri.has_value() || 
                      pending.ct.has_value() || 
                      pending.xy.has_value();
    if (!hasChanges) return true;

    StaticJsonDocument<256> doc;

    if(pending.on.has_value()) doc["on"] = pending.on.value();
    if(pending.bri.has_value()) doc["bri"] = pending.bri.value();
    if(pending.ct.has_value()) doc["ct"] = pending.ct.value();
    if(pending.xy.has_value()) {
        JsonArray xyArr = doc.createNestedArray("xy");
        xyArr.add(pending.xy->first);
        xyArr.add(pending.xy->second);
    }

    String payload;
    serializeJson(doc, payload);

    if(!bridge.sendLightState(_id, payload)) return false;

    // Lokale Werte aktualisieren
    if(pending.on.has_value()) _on = pending.on.value();
    if(pending.bri.has_value()) _bri = pending.bri.value();
    if(pending.ct.has_value()) _ct = pending.ct.value();
    if(pending.xy.has_value()) { 
      _x = pending.xy->first; 
      _y = pending.xy->second; 
    }

    clearPending();
    return true;
}
